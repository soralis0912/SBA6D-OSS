#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/crypto.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/dmapool.h>
#include <net/xfrm.h>
#include <net/esp.h>
#include <crypto/aead.h>
#include <crypto/ctr.h>

#include "cs_adapter.h"
#include "api_driver97_init.h"
#include "api_dmabuf.h"
#include "api_pec.h"
#include "adapter_pecdev_dma.h"
#include "adapter_dmabuf.h"
#include "device_mgmt.h"
#include "lkm.h"
#include "iotoken_ext_97.h"
#include "sa_builder.h"
#include "sa_builder_basic.h"
#include "sa_builder_ipsec.h"
#include "sa_builder_params_ipsec.h"
#include "token_builder.h"
#include "token_builder_internal.h"

#define MTK_CRYPTO_RING0_WQ_NAME           "mtk-crypto-r0-wq"
#define MTK_CRYPTO_FLOW_NUM                32
#define MTK_CRYPTO_SA_WORD_MAX             42
#define MTK_CRYPTO_TOKEN_WORD_MAX          32
#define MTK_CRYPTO_TCRDATA_WORD_MAX        9
#define MTK_CRYPTO_DIGEST_BYTE_MAX         128 /* sha512 block size */
#define MTK_CRYPTO_PACKET_BYTE_TMP         0x1f000u
#define MTK_CRYPTO_DMA_ALIGN_BYTE          4

#define MTK_CRYPTO_SKB_CB(__skb_p__) ((mtk_crypto_espCb_t*)&((__skb_p__)->cb[0]))
#define MTK_CRYPTO_SPIN_LOCK(__lock_p__) spin_lock_bh(__lock_p__)
#define MTK_CRYPTO_SPIN_UNLOCK(__lock_p__) spin_unlock_bh(__lock_p__)

#define MTK_CRYPTO_PRINT(__level__,...) do     \
{                                              \
	if ((__level__) <= mtk_crypto_logLevel)    \
	{                                          \
		printk(__VA_ARGS__);                   \
	}                                          \
}while (0)

typedef enum
{
	MTK_CRYPTO_LOG_ERR  = 1,
	MTK_CRYPTO_LOG_WARN = 2,
	MTK_CRYPTO_LOG_INFO = 3,
} MTK_CRYPTO_LOG_T;

typedef enum
{
	MTK_CRYPTO_APP_IN_DIGEST = 0,
	MTK_CRYPTO_APP_OUT_DIGEST,
	MTK_CRYPTO_APP_DEC,
	MTK_CRYPTO_APP_ENC,
} MTK_CRYPTO_APP_ID;

typedef struct
{
	char                *name_p;
	uint32_t            blockBytes;
	uint32_t            digestBytes;
	SABuilder_Auth_t    hashAlgo;
	SABuilder_Auth_t    hmacAlgo;
} mtk_crypto_authAlgoInfo_t;

typedef struct
{
	char                       *name_p;
	SABuilder_Crypto_t         cryptoAlgo;
	SABuilder_Crypto_Mode_t    cryptoMode;
} mtk_crypto_cipherAlgoInfo_t;

typedef struct
{
	/* SA */
	DMABuf_Handle_t    saHandle;
	uint32_t           SA_WordCount;

	/* Token
	 * -- content would be copied to cmdDesc when put pkt to ring
	 */
	void               *tokenBuf_p;
	uint32_t           Token_WordCount;
	uint32_t           tcr_arr[MTK_CRYPTO_TCRDATA_WORD_MAX];

	/* Token header
	 *  -- content would be copied to cmdDesc when put pkt to ring
	 *  -- shared by in/outDigest precompute,
	 *     app_id would be reassigned after copied to cmdDesc
	 */
	uint32_t           inToken_arr[IOTOKEN_IN_WORD_COUNT];

	/* in/outdigest data buffers */
	DMABuf_Handle_t    inDigestHandle;
	DMABuf_Handle_t    outDigestHandle;
	uint32_t           digestBytes;
} mtk_crypto_hashDigestDesc_t;

typedef struct
{
	uint32_t                    is_decrypt;

	/* SA : would be build after in/outDigest precompute done*/
	SABuilder_Params_t          params;
	SABuilder_Params_IPsec_t    ipsecParams;
	uint32_t                    SA_WordCount;
	DMABuf_Handle_t             saHandle;

	/* Token
	 * -- content would be copied to cmdDesc when put pkt to ring
	 * -- pkt len/next hdr/padding len would be reassigned per pkt after copied to cmdDesc
	 */
	void                        *tokenBuf_p;
	uint32_t                    Token_WordCount;
	uint32_t                    tcr_arr[MTK_CRYPTO_TCRDATA_WORD_MAX];
	uint32_t                    padBlockBytes;
	uint32_t                    dirTokenIdx;
	uint32_t                    insNxtPadTokenIdx;
	uint32_t                    pktTmpBytes;

	/* Token header
	 *  -- content would be copied to cmdDesc when put pkt to ring
	 *  -- pkt len would be reassigned after copied to cmdDesc
	 */
	uint32_t                    inToken_arr[IOTOKEN_IN_WORD_COUNT];
} mtk_crypto_espDesc_t;

typedef struct
{
	uint32_t    inPktTot;    /* receive pkt cnt from xfrm_input() or xfrm_output() */
	uint32_t    inPktDrop;

	uint32_t    queuePutTot;
	uint32_t    queuePutDrop;

	uint32_t    ringGetTot;
	uint32_t    ringGetAppFail;
	uint32_t    ringGetSeqFail;
	uint32_t    ringGetOtherFail;

	uint32_t    expandCopy;
	uint32_t    linearize;
} mtk_crypto_flowCounter_t;

typedef struct
{
	spinlock_t                     lock;

	struct xfrm_state              *x_p;  //null means invalid
	uint32_t                       spi;   //every ipsec flow has a unique spi
	mtk_crypto_authAlgoInfo_t      *hashAlgoInfo_p;
	mtk_crypto_cipherAlgoInfo_t    *cipherAlgoInfo_p;

	uint32_t                       tobe_del;
	mtk_crypto_hashDigestDesc_t    hashDigestDesc;
	mtk_crypto_espDesc_t           espDesc;
	uint32_t                       digest_bmp;
	uint32_t                       digestDone_bmp;
	struct sk_buff_head            skbQueue;
	mtk_crypto_flowCounter_t       counter;
} mtk_crypto_flowInfo_t;

typedef struct
{
	struct xfrm_skb_cb    xfrm;
	void                  *flowInfo_p;
} mtk_crypto_espCb_t; /* current size is 48B, can not add more members */

#if IS_REACHABLE(CONFIG_INET_ESP)
extern int (*crypto_esp_sendReq_func_p)(struct xfrm_state *x_p, struct sk_buff *skb_p,
										unsigned int is_decrypt);
#endif

//#if IS_REACHABLE(CONFIG_INET6_ESP)
//extern int (*crypto_esp6_sendReq_func_p)(struct xfrm_state *x_p, struct sk_buff *skb_p,
//										   unsigned int is_decrypt);
//#endif

extern void (*crypto_deleteFlow_func_p)(unsigned int spi);

static atomic_t                      mtk_crypto_en = {0};
static atomic_t                      mtk_crypto_eip97_en = {0}; /* call LPM_EIP97_Enable(1) to set vcore/clock/lock */
static int                           mtk_crypto_logLevel = MTK_CRYPTO_LOG_WARN;

struct dma_pool                      *mtk_crypto_sa_pool;     /* used @esp/hashDigest desc */
struct dma_pool                      *mtk_crypto_digest_pool; /* in/outDigest data used @hashDigest desc */
struct dma_pool                      *mtk_crypto_token_pool;  /* used @cmdDesc desc */

static mtk_crypto_flowInfo_t         mtk_crypto_decFlow_arr[MTK_CRYPTO_FLOW_NUM];
static mtk_crypto_flowInfo_t         mtk_crypto_encFlow_arr[MTK_CRYPTO_FLOW_NUM];
static spinlock_t                    mtk_crypto_decFlow_lock;
static spinlock_t                    mtk_crypto_encFlow_lock;

static PEC_CommandDescriptor_t       *mtk_crypto_cmdDesc_arr = NULL;
static PEC_ResultDescriptor_t        *mtk_crypto_rsltDesc_arr = NULL;
static uint32_t                      mtk_crypto_cmdDesc_idx = 0;

static struct workqueue_struct       *mtk_crypto_ring0_wq_p;
static struct work_struct            mtk_crypto_ring0PutPkt_work;
struct sk_buff_head                  mtk_crypto_skbQueue;
static uint8_t                       mtk_crypto_dummyAuthKey[128] = {0};

static int                           mtk_crypto_singlePutMax = 32;
static int                           mtk_crypto_singleGetMax = 32;

static struct proc_dir_entry         *mtk_crypto_procDir;
static struct proc_dir_entry         *mtk_crypto_enEip97_procDir;
static struct proc_dir_entry         *mtk_crypto_enLog_procDir;
static struct proc_dir_entry         *mtk_crypto_dbg_procDir;

static mtk_crypto_authAlgoInfo_t     mtk_crypto_authAlgo_arr[] =
{
	{"hmac(sha1)",   64, 20, SAB_AUTH_HASH_SHA1, SAB_AUTH_HMAC_SHA1},
	//{"hmac(sha224)", 64, 28, SAB_AUTH_HASH_SHA2_224, SAB_AUTH_HMAC_SHA2_224},
	{"hmac(sha256)", 64, 32, SAB_AUTH_HASH_SHA2_256, SAB_AUTH_HMAC_SHA2_256},
	//{"hmac(sha384)", 128, 48, SAB_AUTH_HASH_SHA2_384, SAB_AUTH_HMAC_SHA2_384},
	{"hmac(sha512)", 128, 64, SAB_AUTH_HASH_SHA2_512, SAB_AUTH_HMAC_SHA2_512},
};

static mtk_crypto_cipherAlgoInfo_t    mtk_crypto_cipherAlgo_arr[] =
{
	{"cbc(aes)", SAB_CRYPTO_AES,  SAB_CRYPTO_MODE_CBC},
	//{"ctr(aes)", SAB_CRYPTO_AES,  SAB_CRYPTO_MODE_CTR},
};

static void
mtk_crypto_clearFlowDb(
	mtk_crypto_flowInfo_t    *flowInfo_p)
{
	MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
		"[mtk-crypto][info] clear spi 0x%x traffic flow\n", flowInfo_p->spi);

	flowInfo_p->digest_bmp = 0;
	flowInfo_p->digestDone_bmp = 0;

	memset(&(flowInfo_p->counter), 0, sizeof(mtk_crypto_flowCounter_t));
	skb_queue_purge(&(flowInfo_p->skbQueue));

	flowInfo_p->tobe_del = 0;
	flowInfo_p->spi = 0;
	flowInfo_p->x_p = NULL;
}

static unsigned int
mtk_crypto_PadBlockSize(
	unsigned int    DefaultVal,
	unsigned int    OverrideVal)
{
	/* reference to PadBlockSize @ token_builder_macros.h */
	if (OverrideVal > DefaultVal  &&
		OverrideVal <= 256 &&
		(OverrideVal  & (OverrideVal - 1)) == 0) // Check power of two.
	{
		return OverrideVal;
	}
	else
	{
		return DefaultVal;
	}
}


static unsigned int
mtk_crypto_PaddedSize(
	unsigned int    ByteCount,
	unsigned int    BlockByteCount)
{
	/* reference to PaddedSize @ token_builder_macros.h */
	return (ByteCount + BlockByteCount - 1) & ~(BlockByteCount - 1);
}

static unsigned int
mtk_crypto_ComputePadBytes(
	const TokenBuilder_Context_t    *TokenContext_p,
	unsigned int                    PayloadByteCount,
	unsigned int                    PadBlockByteCount)
{
	/* reference to ComputePadBytes @ token_builder_macros.h */
	unsigned int PadByteCount;
	if(TokenContext_p->protocol == TKB_PROTO_SSLTLS_OUT ||
		TokenContext_p->protocol == TKB_PROTO_BASIC_HASHENC)
	{
		PadByteCount = mtk_crypto_PaddedSize(PayloadByteCount + 1 +
								  TokenContext_p->IVByteCount +
								  TokenContext_p->ICVByteCount,
								  PadBlockByteCount) -
		  PayloadByteCount - TokenContext_p->ICVByteCount -
		  TokenContext_p->IVByteCount;
	}
	else
	{
		PadByteCount =  mtk_crypto_PaddedSize(PayloadByteCount + 2, PadBlockByteCount) -
			PayloadByteCount;
	}
	return PadByteCount;
}

static int
mtk_crypto_handleHashRslt(
	PEC_ResultDescriptor_t    *rsltDesc_p,
	mtk_crypto_flowInfo_t     *flowInfo_p,
	uint32_t                  is_inner_digest,
	uint32_t                  interfaceId)
{
	int     rc = 0;
	void    *SABuf_p;

	if (is_inner_digest)
	{
		flowInfo_p->espDesc.params.AuthKey1_p = rsltDesc_p->DstPkt_p;
	}
	else
	{
		flowInfo_p->espDesc.params.AuthKey2_p = rsltDesc_p->DstPkt_p;

		SABuf_p = Adapter_DMAResource_HostAddr(
			Adapter_DMABuf_Handle2DMAResourceHandle(flowInfo_p->espDesc.saHandle));

		rc = SABuilder_BuildSA(&(flowInfo_p->espDesc.params),
							   (uint32_t *)SABuf_p, NULL, NULL);
		if (rc == 0)
		{
			rc = PEC_SA_Register(interfaceId, flowInfo_p->espDesc.saHandle,
								 DMABuf_NULLHandle, DMABuf_NULLHandle);
			if (rc != 0)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] PEC_SA_Register failed, rc = %d\n", rc);
			}
		}
		else
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] SABuilder_BuildSA failed, rc = %d\n", rc);
		}
	}

	return rc;
}

static void
mtk_crypto_handleEspRslt(
	PEC_ResultDescriptor_t    *rsltDesc_p,
	uint32_t                  is_decrypt,
	struct xfrm_state         *x_p,
	struct sk_buff            *skb_p)
{
	struct udphdr *uh;
	struct ip_esp_hdr *esph;
	xfrm_address_t ipaddr;
	__be32 *udpdata32;
	int ihl;

	dma_sync_single_for_cpu(Device_GetReference(NULL, NULL),
							virt_to_phys(rsltDesc_p->DstPkt_p),
							rsltDesc_p->DstPkt_ByteCount, DMA_FROM_DEVICE);
	if (is_decrypt)
	{
		/* not support ESN */

		skb_p->len = rsltDesc_p->DstPkt_ByteCount;
		skb_set_tail_pointer(skb_p, rsltDesc_p->DstPkt_ByteCount);

		if (skb_p->protocol == htons(ETH_P_IP))
		{
			ihl = ip_hdr(skb_p)->ihl * 4;

			if (x_p->encap)
			{
				uh = (void *)(skb_network_header(skb_p) + ihl);

				if (ip_hdr(skb_p)->saddr != x_p->props.saddr.a4 ||
					uh->source != x_p->encap->encap_sport)
				{
					ipaddr.a4 = ip_hdr(skb_p)->saddr;
					km_new_mapping(x_p, &ipaddr, uh->source);
				}

				if (x_p->props.mode == XFRM_MODE_TRANSPORT)
				{
					skb_p->ip_summed = CHECKSUM_UNNECESSARY;
				}
			}
		}
		else
		{
			ihl = skb_network_header_len(skb_p);
			/* kernel 5.4.123 code do not handle NAT-T @ v6 scenario */
		}

		if (x_p->props.mode == XFRM_MODE_TRANSPORT)
		{
			skb_set_transport_header(skb_p, -ihl);
		}
		else
		{
			skb_reset_transport_header(skb_p);
		}
	}
	else
	{
		/* not support ESN/XFRM_DEV_RESUME handling */

		skb_put(skb_p, rsltDesc_p->DstPkt_ByteCount - skb_p->len);

		if (skb_p->protocol == htons(ETH_P_IP))
		{
			ip_hdr(skb_p)->tot_len = rsltDesc_p->DstPkt_ByteCount +
									 (skb_p->data - skb_network_header(skb_p));
			__skb_push(skb_p, -skb_network_offset(skb_p));
			*skb_mac_header(skb_p) = IPPROTO_ESP;

			if (x_p->encap)
			{
				esph = ip_esp_hdr(skb_p);
				uh = (struct udphdr *)esph;

				uh->source = x_p->encap->encap_sport;
				uh->dest = x_p->encap->encap_dport;
				uh->len = htons(skb_p->len - skb_transport_offset(skb_p));
				uh->check = 0;

				if (x_p->encap->encap_type == UDP_ENCAP_ESPINUDP_NON_IKE)
				{
					udpdata32 = (__be32 *)(uh + 1);
					udpdata32[0] = udpdata32[1] = 0;
				}

				*skb_mac_header(skb_p) = IPPROTO_UDP;
			}
		}
		else
		{
			ipv6_hdr(skb_p)->payload_len = rsltDesc_p->DstPkt_ByteCount;
			__skb_push(skb_p, -skb_network_offset(skb_p));
			*skb_mac_header(skb_p) = IPPROTO_ESP;
			/* kernel 5.4.123 code do not handle NAT-T @ v6 scenario */
		}
	}
}

void
mtk_crypto_getRing0Pkt(void)
{
	int                       rc;
	uint32_t                  interfaceId = 0, get_cnt = 0, cnt = 0, notify_wq;
	PEC_ResultDescriptor_t    *rsltDesc_p = mtk_crypto_rsltDesc_arr;
	IOToken_Output_Dscr_t     OutTokenDesc;
	mtk_crypto_flowInfo_t     *flowInfo_p;
	struct sk_buff            *skb_p;

	while (1)
	{
		rsltDesc_p = mtk_crypto_rsltDesc_arr;

		rc = PEC_Packet_Get(interfaceId, rsltDesc_p, mtk_crypto_singleGetMax, &get_cnt);

		if ((rc != 0) || (get_cnt == 0))
		{
			break;
		}
		else
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
						 "[mtk-crypto][info] Get %d packet from rdr%d\n",
						 get_cnt, interfaceId);

			local_bh_disable();

			cnt = 0;
			while (cnt < get_cnt)
			{
				IOToken_Parse(rsltDesc_p->OutputToken_p, &OutTokenDesc);

				skb_p = rsltDesc_p->User_p;
				flowInfo_p = MTK_CRYPTO_SKB_CB(skb_p)->flowInfo_p;

				if (OutTokenDesc.ErrorCode)
				{
					if (OutTokenDesc.ErrorCode == 0x400)
					{
						flowInfo_p->counter.ringGetSeqFail++;
					}
					else
					{
						flowInfo_p->counter.ringGetOtherFail++;

						MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
							"[mtk-crypto][warn] Pkt process fail, app %d, err-code 0x%x, dst-pkt-len %d\n",
							OutTokenDesc.AppID, OutTokenDesc.ErrorCode, rsltDesc_p->DstPkt_ByteCount);
						MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
							"[mtk-crypto][warn] Pkt process fail, OutToken = %x.%x.%x.%x\n",
							rsltDesc_p->OutputToken_p[3], rsltDesc_p->OutputToken_p[2],
							rsltDesc_p->OutputToken_p[1], rsltDesc_p->OutputToken_p[0]);
					}

					if ((OutTokenDesc.AppID == MTK_CRYPTO_APP_IN_DIGEST) ||
						(OutTokenDesc.AppID == MTK_CRYPTO_APP_OUT_DIGEST))
					{
						kfree(skb_p);
					}
					else if ((OutTokenDesc.AppID == MTK_CRYPTO_APP_DEC) ||
							 (OutTokenDesc.AppID == MTK_CRYPTO_APP_ENC))
					{
						kfree_skb(skb_p);
						DMABuf_Release(rsltDesc_p->SrcPkt_Handle);
					}
				}
				else
				{
					if (OutTokenDesc.AppID == MTK_CRYPTO_APP_IN_DIGEST)
					{
						mtk_crypto_handleHashRslt(rsltDesc_p, flowInfo_p, 1, interfaceId);
						if (rc == 0)
						{
							MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);
							flowInfo_p->digestDone_bmp |= (1 << 0);
							MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);
						}
						kfree(skb_p);
					}
					else if (OutTokenDesc.AppID == MTK_CRYPTO_APP_OUT_DIGEST)
					{
						mtk_crypto_handleHashRslt(rsltDesc_p, flowInfo_p, 0, interfaceId);
						if (rc == 0)
						{
							MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

							flowInfo_p->digestDone_bmp |= (1 << 1);
							if (flowInfo_p->digestDone_bmp == 0x3)
							{
								// no need to take flowInfo_p->skbQueue.lock
								// (it is protected by flowInfo_p->lock)
								MTK_CRYPTO_SPIN_LOCK(&(mtk_crypto_skbQueue.lock));
								flowInfo_p->counter.queuePutTot += skb_queue_len(&flowInfo_p->skbQueue);
								skb_queue_splice_tail_init(&flowInfo_p->skbQueue, &mtk_crypto_skbQueue);
								MTK_CRYPTO_SPIN_UNLOCK(&(mtk_crypto_skbQueue.lock));

							    notify_wq = 1;
							}
							else
							{
								notify_wq = 0;
							}

							MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);

							if (notify_wq)
							{
								queue_work(mtk_crypto_ring0_wq_p, &mtk_crypto_ring0PutPkt_work);
							}
						}
						kfree(skb_p);
					}
					else if (OutTokenDesc.AppID == MTK_CRYPTO_APP_DEC)
					{
						mtk_crypto_handleEspRslt(rsltDesc_p, 1, flowInfo_p->x_p, skb_p);

						//clear rps hash for tunnel term pkt
						skb_clear_hash(skb_p);
						xfrm_input_resume(skb_p, OutTokenDesc.NextHeader);

						DMABuf_Release(rsltDesc_p->SrcPkt_Handle);
					}
					else if (OutTokenDesc.AppID == MTK_CRYPTO_APP_ENC)
					{
						mtk_crypto_handleEspRslt(rsltDesc_p, 0, flowInfo_p->x_p, skb_p);

						xfrm_output_resume(skb_p, 0);

						DMABuf_Release(rsltDesc_p->SrcPkt_Handle);
					}
					else
					{
						flowInfo_p->counter.ringGetAppFail++;
						MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
							"[mtk-crypto][err] get pkt with unexpect app %d\n",
							OutTokenDesc.AppID);
					}
				}

				MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

				flowInfo_p->counter.ringGetTot++;

				if ((flowInfo_p->tobe_del) &&
					(flowInfo_p->counter.ringGetTot + flowInfo_p->counter.queuePutDrop
					 == flowInfo_p->counter.queuePutTot))
				{
					mtk_crypto_clearFlowDb(flowInfo_p);
				}

				MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);

				rsltDesc_p++;
				cnt++;
			}

			local_bh_enable();
		}
	}

	if (PEC_STATUS_OK != PEC_ResultNotify_Request(0, mtk_crypto_getRing0Pkt, 1))
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
			"[mtk-crypto][err] request PEC ring %d result notify fail\n",
			interfaceId);
	}
}

static void
mtk_crypto_buildHashCmd(
	mtk_crypto_hashDigestDesc_t    *hashDigestDesc_p,
	struct sk_buff                 *skb_p,
	uint32_t                       is_inner_digest,
	PEC_CommandDescriptor_t        *cmdDesc_p)  /* output */
{
	uint32_t    *dsttokenBuf_p;

	cmdDesc_p->User_p = skb_p;
	cmdDesc_p->Token_WordCount = hashDigestDesc_p->Token_WordCount;

	dsttokenBuf_p = Adapter_DMAResource_HostAddr(
		Adapter_DMABuf_Handle2DMAResourceHandle(cmdDesc_p->Token_Handle));
	memcpy(dsttokenBuf_p, hashDigestDesc_p->tokenBuf_p,
		   hashDigestDesc_p->Token_WordCount * 4);

	cmdDesc_p->SA_Handle1 = hashDigestDesc_p->saHandle;
	cmdDesc_p->SA_Handle2 = DMABuf_NULLHandle;
	cmdDesc_p->SA_WordCount = hashDigestDesc_p->SA_WordCount;
	cmdDesc_p->Control1 = 0;
	cmdDesc_p->Control2 = 0;
	cmdDesc_p->Bypass_WordCount = 0;

	memcpy(cmdDesc_p->InputToken_p, hashDigestDesc_p->inToken_arr,
		   sizeof(hashDigestDesc_p->inToken_arr));

	if (is_inner_digest)
	{
		cmdDesc_p->InputToken_p[1] =
			(cmdDesc_p->InputToken_p[1] & (~0xfe00)) |
			(MTK_CRYPTO_APP_IN_DIGEST << 9);

		cmdDesc_p->SrcPkt_ByteCount = hashDigestDesc_p->digestBytes;
		cmdDesc_p->SrcPkt_Handle = hashDigestDesc_p->inDigestHandle;
		cmdDesc_p->DstPkt_Handle = hashDigestDesc_p->inDigestHandle;
	}
	else
	{
		cmdDesc_p->InputToken_p[1] =
			(cmdDesc_p->InputToken_p[1] & (~0xfe00)) |
			(MTK_CRYPTO_APP_OUT_DIGEST << 9);

		cmdDesc_p->SrcPkt_ByteCount = hashDigestDesc_p->digestBytes;
		cmdDesc_p->SrcPkt_Handle = hashDigestDesc_p->outDigestHandle;
		cmdDesc_p->DstPkt_Handle = hashDigestDesc_p->outDigestHandle;
	}
}

static int
mtk_crypto_buildEspCmd(
	mtk_crypto_espDesc_t       *espDesc_p,
	struct sk_buff             **skb_pp,
	PEC_CommandDescriptor_t    *cmdDesc_p,    /* output */
	uint32_t                   *do_expandCopy_p)
{
	int                       rc = 0;
	TokenBuilder_Context_t    *tokenContext_p = (TokenBuilder_Context_t*)(espDesc_p->tcr_arr);
	DMABuf_Properties_t       DMAProperties = {false, MTK_CRYPTO_DMA_ALIGN_BYTE, 0, 0};
	dma_addr_t                dma_addr;
	struct sk_buff            *skb_tmp_p;
	uint32_t                  *dsttokenBuf_p;
	uint32_t                  PadBytes = 0, AddBytes = 0;

	*do_expandCopy_p = 0;

	cmdDesc_p->SA_Handle1 = espDesc_p->saHandle;
	cmdDesc_p->SA_Handle2 = DMABuf_NULLHandle;
	cmdDesc_p->SA_WordCount = espDesc_p->SA_WordCount;
	cmdDesc_p->Control1 = 0;
	cmdDesc_p->Control2 = 0;
	cmdDesc_p->Bypass_WordCount = 0;

	memcpy(cmdDesc_p->InputToken_p, espDesc_p->inToken_arr,
		   sizeof(espDesc_p->inToken_arr));
	cmdDesc_p->InputToken_p[0] =
		   (cmdDesc_p->InputToken_p[0] & ~0x1ffff) | (*skb_pp)->len;

	cmdDesc_p->Token_WordCount = espDesc_p->Token_WordCount;

	dsttokenBuf_p = Adapter_DMAResource_HostAddr(
		Adapter_DMABuf_Handle2DMAResourceHandle(cmdDesc_p->Token_Handle));
	memcpy(dsttokenBuf_p, espDesc_p->tokenBuf_p,
		   espDesc_p->Token_WordCount * 4);

	dsttokenBuf_p[espDesc_p->dirTokenIdx] =
			dsttokenBuf_p[espDesc_p->dirTokenIdx] + (*skb_pp)->len - espDesc_p->pktTmpBytes;

	if (0 == espDesc_p->is_decrypt)
	{
		PadBytes = mtk_crypto_ComputePadBytes(tokenContext_p, (*skb_pp)->len, espDesc_p->padBlockBytes);

		if ((*skb_pp)->protocol == htons(ETH_P_IP))
		{
			dsttokenBuf_p[espDesc_p->insNxtPadTokenIdx] =
				(dsttokenBuf_p[espDesc_p->insNxtPadTokenIdx] & (~0x1ffff)) |
				((ip_hdr(*skb_pp)->protocol << 9) | PadBytes);
		}
		else
		{
			dsttokenBuf_p[espDesc_p->insNxtPadTokenIdx] =
				(dsttokenBuf_p[espDesc_p->insNxtPadTokenIdx] & (~0x1ffff)) |
				((ipv6_hdr(*skb_pp)->nexthdr << 9) | PadBytes);
		}

		AddBytes = 8 + tokenContext_p->IVByteCount +
				   PadBytes + tokenContext_p->ICVByteCount;

		if (skb_tailroom(*skb_pp) < AddBytes)
		{
			skb_tmp_p = skb_copy_expand((*skb_pp), skb_headroom(*skb_pp), AddBytes, GFP_ATOMIC);
			if (NULL != skb_tmp_p)
			{
				kfree_skb(*skb_pp); //free old skb
				(*skb_pp) = skb_tmp_p;

				*do_expandCopy_p = 1;
			}
			else
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] expand skb fail, add bytes = %d\n", AddBytes);
				rc = -1;
			}
		}
	}

	if (rc == 0)
	{
		cmdDesc_p->User_p = (*skb_pp);

		dma_addr = virt_to_phys((*skb_pp)->data);
		dma_sync_single_for_device(Device_GetReference(NULL, NULL),
						dma_addr, (*skb_pp)->len + AddBytes, DMA_TO_DEVICE);

		/* DMAProperties.Size should be the max size of Src/Dst packet */
		DMAProperties.Size = (*skb_pp)->len + AddBytes;
		rc = DMABuf_Register(DMAProperties, (*skb_pp)->data,
				(void *)(uintptr_t)(dma_addr), 'C', &(cmdDesc_p->SrcPkt_Handle));
		if (rc == 0)
		{
			cmdDesc_p->DstPkt_Handle = cmdDesc_p->SrcPkt_Handle;
			cmdDesc_p->SrcPkt_ByteCount= (*skb_pp)->len;
		}
		else
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
							 "[mtk-crypto][warn] register skb handle fail\n");
		}
	}

	return rc;
}

void
mtk_crypto_putRing0Pkt(
	struct work_struct    *work_p)
{
	int                        rc = 0;
	int                        pkt_cnt, free_cnt, put_cnt, actual_put_cnt, cnt = -1;
	uint32_t                   interfaceId = 0, ring_full = 0, do_expandCopy;
	uint32_t                   cmdDesc_start_idx;
	mtk_crypto_flowInfo_t      *flowInfo_p;
	PEC_CommandDescriptor_t    *cmdDesc_p;
	struct sk_buff             *skb_p;

	while (1)
	{
		MTK_CRYPTO_SPIN_LOCK(&(mtk_crypto_skbQueue.lock));
		pkt_cnt = skb_queue_len(&mtk_crypto_skbQueue);
		MTK_CRYPTO_SPIN_UNLOCK(&(mtk_crypto_skbQueue.lock));

		if (pkt_cnt)
		{
			free_cnt = Adapter_PECDev_GetFreeSpace(interfaceId);
			if (pkt_cnt > free_cnt)
			{
				ring_full = 1;
				pkt_cnt = free_cnt;
			}
		}
		else
		{
			break;
		}

		cnt = 0;
		put_cnt = 0;
		cmdDesc_start_idx = mtk_crypto_cmdDesc_idx;
		while (cnt < pkt_cnt)
		{
			rc = 0;
			cmdDesc_p = mtk_crypto_cmdDesc_arr + mtk_crypto_cmdDesc_idx;

			skb_p = skb_dequeue(&mtk_crypto_skbQueue);
			if (NULL == skb_p)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
					"[mtk-crypto][err] dequeue skb fail, cnt=%d, pkt_cnt=%d\n",
					cnt, pkt_cnt);
				rc = -1;

				break;
			}
			else
			{
				flowInfo_p = MTK_CRYPTO_SKB_CB(skb_p)->flowInfo_p;

				if ((flowInfo_p->digest_bmp & (1 << 0)) == 0) /* inner hash digest */
				{
					mtk_crypto_buildHashCmd(&(flowInfo_p->hashDigestDesc), skb_p,
											1, cmdDesc_p);
					flowInfo_p->digest_bmp |= (1 << 0);
				}
				else if ((flowInfo_p->digest_bmp & (1 << 1)) == 0) /* outer hash digest */
				{
					mtk_crypto_buildHashCmd(&(flowInfo_p->hashDigestDesc), skb_p,
											0, cmdDesc_p);
					flowInfo_p->digest_bmp |= (1 << 1);
				}
				else /* esp enc/dec */
				{
					rc = mtk_crypto_buildEspCmd(&(flowInfo_p->espDesc), &skb_p,
						                        cmdDesc_p, &do_expandCopy);
					if (do_expandCopy)
					{
						flowInfo_p->counter.expandCopy++;
					}
					if (rc != 0)
					{
						MTK_CRYPTO_SPIN_LOCK(&(flowInfo_p->lock));
						flowInfo_p->counter.queuePutDrop++;
						MTK_CRYPTO_SPIN_UNLOCK(&(flowInfo_p->lock));

						kfree_skb(skb_p);
						MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
										 "[mtk-crypto][warn] build esp cmd desc fail\n");
					}
				}

				if (rc == 0)
				{
					put_cnt++;
					mtk_crypto_cmdDesc_idx++;
					if (mtk_crypto_cmdDesc_idx >= DRIVER_PEC_MAX_PACKETS)
					{
						mtk_crypto_cmdDesc_idx = 0;
					}

					if (put_cnt)
					{
						if ((mtk_crypto_cmdDesc_idx == 0) || (put_cnt >= mtk_crypto_singlePutMax))
						{
							rc = PEC_Packet_Put(interfaceId, mtk_crypto_cmdDesc_arr + cmdDesc_start_idx,
												put_cnt, &actual_put_cnt);
							if (rc == 0)
							{
								if (actual_put_cnt != put_cnt)
								{
									MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
										"[mtk-crypto][err] Put packet to cdr%d fail, expect put %d, actual put %d, cmdDesc idx %d\n",
										interfaceId, put_cnt, actual_put_cnt, cmdDesc_start_idx);
								}
								else
								{
									MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
													 "[mtk-crypto][info] Put %d packet to cdr%d\n",
													 put_cnt, interfaceId);
								}
							}
							else
							{
								MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
									"[mtk-crypto][err] Put packet to cdr%d fail, rc=%d, cnt=%d, cmdDesc idx %d\n",
									interfaceId, rc, put_cnt, cmdDesc_start_idx);
							}

							put_cnt = 0;
							cmdDesc_start_idx = mtk_crypto_cmdDesc_idx;
						}
					}
				}
			}

			cnt++;
		}

		if (put_cnt)
		{
			rc = PEC_Packet_Put(interfaceId, mtk_crypto_cmdDesc_arr + cmdDesc_start_idx,
								put_cnt, &actual_put_cnt);
			if (rc == 0)
			{
				if (actual_put_cnt != put_cnt)
				{
					MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
						"[mtk-crypto][err] Put packet to cdr%d fail, expect put %d, actual put %d, cmdDesc idx %d\n",
						interfaceId, put_cnt, actual_put_cnt, cmdDesc_start_idx);
				}
				else
				{
					MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
									 "[mtk-crypto][info] Put %d packet to cdr%d\n",
									 put_cnt, interfaceId);
				}
			}
			else
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_ERR,
					"[mtk-crypto][err] Put packet to cdr%d fail, rc=%d, cnt=%d, cmdDesc idx %d\n",
					interfaceId, rc, put_cnt, cmdDesc_start_idx);
			}
		}

		if (ring_full)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
						 "[mtk-crypto][info] ring %d full\n", interfaceId);
			break;
		}
	}
}

static int
mtk_crypto_checkXfrmState(
	struct xfrm_state    *x_p)
{
	if (x_p->props.flags & XFRM_STATE_ESN)
	{
		return -1;
	}
	else if ((x_p->encap == NULL) ||
			 ((x_p->encap != NULL) &&
			 (x_p->encap->encap_type == UDP_ENCAP_ESPINUDP_NON_IKE)))
	{
		/* transport mode or tunnel mode with espinudp_non_ike */
		return -1;
	}

	return 0;
}

static int
mtk_crypto_checkSkb(
	struct xfrm_state    *x_p,
	struct sk_buff       *skb_p,
	uint32_t             is_decrypt,
	uint32_t             *do_linearize_p)
{
	int    esp_header_len = sizeof(struct ip_esp_hdr) +
							crypto_aead_ivsize((struct crypto_aead*)(x_p->data));

	*do_linearize_p = 0;

	if (is_decrypt)
	{
		if ((!pskb_may_pull(skb_p, esp_header_len)) ||
			(skb_p->len <= esp_header_len))
		{
			return -1;
		}

		/* not support ESN */

		if (skb_is_nonlinear(skb_p))
		{
			if (skb_linearize(skb_p))
			{
				return -1;
			}
			*do_linearize_p = 1;
		}

		skb_p->ip_summed = CHECKSUM_NONE;
	}
	else
	{
		/* Since eip97 would add esp hdr by itself,
		 * we need to remove the esp hdr added by xfrm4/6_tunnel_encap_add()
		 * (before memmove)
		 * -- tunnel mode    : ip + udp + esp + ip + tcp
		 * -- transport mode : ip + esp + tcp
		 * (after memmove)
		 * -- tunnel mode : ip + udp + ip + tcp
		 * -- transport mode : ip + tcp
		 */
		memmove(skb_network_header(skb_p) + esp_header_len, skb_network_header(skb_p),
				x_p->props.header_len - esp_header_len);
		skb_p->mac_header += esp_header_len;
		skb_p->network_header += esp_header_len;
		skb_p->transport_header += esp_header_len;

		/* not support ESN */

		if (skb_is_nonlinear(skb_p))
		{
			if (skb_linearize(skb_p))
			{
				return -1;
			}
			*do_linearize_p = 1;
		}
	}

	return 0;
}

static int
mtk_crypto_getAlgoInfo(
	struct xfrm_state              *x_p,
	mtk_crypto_authAlgoInfo_t      **authAlgoInfo_pp,
	mtk_crypto_cipherAlgoInfo_t    **cipherAlgoInfo_pp)
{
	uint32_t    idx, num;

	*authAlgoInfo_pp = NULL;
	*cipherAlgoInfo_pp = NULL;

	if (NULL != x_p->aalg)
	{
		num = sizeof(mtk_crypto_authAlgo_arr) / sizeof(mtk_crypto_authAlgo_arr[0]);
		for (idx = 0; idx < num; idx++)
		{
			if (0 == strcmp(x_p->aalg->alg_name, mtk_crypto_authAlgo_arr[idx].name_p))
			{
				*authAlgoInfo_pp = &(mtk_crypto_authAlgo_arr[idx]);
			}
		}

		if (NULL == *authAlgoInfo_pp)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] not support auth algo %s\n", x_p->aalg->alg_name);
			return -1;
		}
	}
	else
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN, "[mtk-crypto][warn] auth algo is null\n");
		return -1;
	}

	if (NULL != x_p->ealg)
	{
		num = sizeof(mtk_crypto_cipherAlgo_arr) / sizeof(mtk_crypto_cipherAlgo_arr[0]);
		for (idx = 0; idx < num; idx++)
		{
			if (0 == strcmp(x_p->ealg->alg_name, mtk_crypto_cipherAlgo_arr[idx].name_p))
			{
				*cipherAlgoInfo_pp = &(mtk_crypto_cipherAlgo_arr[idx]);
			}
		}

		if (NULL == *cipherAlgoInfo_pp)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] not support cipher algo %s\n", x_p->ealg->alg_name);
			return -1;
		}
	}
	else
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN, "[mtk-crypto][warn] cipher algo is null\n");
		return -1;
	}

	return 0;
}

static int
mtk_crypto_allocFlowInfo(
	struct xfrm_state        *x_p,
	uint32_t                 is_decrypt,
	mtk_crypto_flowInfo_t    **flowInfo_pp,
	uint32_t                 *is_newFlow_p)
{
	int                      rc = 0;
	mtk_crypto_flowInfo_t    *flowInfo_arr;
	spinlock_t               *lock_p;
	uint32_t                 idx;

	*flowInfo_pp = NULL;

	if (is_decrypt)
	{
		lock_p = &mtk_crypto_decFlow_lock;
		flowInfo_arr = mtk_crypto_decFlow_arr;
	}
	else
	{
		lock_p = &mtk_crypto_encFlow_lock;
		flowInfo_arr = mtk_crypto_encFlow_arr;
	}

	MTK_CRYPTO_SPIN_LOCK(lock_p);

	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if (flowInfo_arr[idx].x_p != NULL)
		{
			if (flowInfo_arr[idx].spi == x_p->id.spi)
			{
				(*flowInfo_pp) = flowInfo_arr + idx;
				*is_newFlow_p = 0;

				break;
			}
		}
		else if (NULL == (*flowInfo_pp))
		{
			(*flowInfo_pp) = flowInfo_arr + idx;
			*is_newFlow_p = 1;
		}
	}

	if (NULL == (*flowInfo_pp))
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] allocate flow-id for spi 0x%x traffic fail (max support %d flow)\n",
			x_p->id.spi, MTK_CRYPTO_FLOW_NUM);

		rc = -1;
	}
	else if (1 == (*is_newFlow_p))
	{
		MTK_CRYPTO_SPIN_LOCK(&((*flowInfo_pp)->lock));

		rc = mtk_crypto_getAlgoInfo(x_p, &((*flowInfo_pp)->hashAlgoInfo_p),
									&((*flowInfo_pp)->cipherAlgoInfo_p));
		if (rc == 0)
		{
			if (((x_p->aalg->alg_key_len + 7) / 8) > (*flowInfo_pp)->hashAlgoInfo_p->blockBytes)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
						"[mtk-crypto][warn][hash] not support hash key bytes %d > block bytes %d\n",
						((x_p->aalg->alg_key_len + 7) / 8),
						(*flowInfo_pp)->hashAlgoInfo_p->blockBytes);
				rc = -1;
			}
			else
			{
				(*flowInfo_pp)->x_p = x_p;
				(*flowInfo_pp)->spi = x_p->id.spi;

				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
					"[mtk-crypto][info] allocate flow-id for spi 0x%x traffic\n",
					x_p->id.spi);
			}
		}

		MTK_CRYPTO_SPIN_UNLOCK(&((*flowInfo_pp)->lock));
	}

	MTK_CRYPTO_SPIN_UNLOCK(lock_p);

	return rc;
}

static int
mtk_crypto_buildHashDsec(
	struct xfrm_state               *x_p,
	mtk_crypto_authAlgoInfo_t       *hashAlgoInfo_p,
	mtk_crypto_hashDigestDesc_t     *hashDigestDesc_p)
{
	int                         rc = 0;
	uint32_t                    interfaceId = 0, idx, words, hashKeyBytes;
	SABuilder_Params_t          params;
	SABuilder_Params_Basic_t    protocolParams;
	TokenBuilder_Params_t       tokenParams;
	uint32_t                    tokenMaxWords, tokenHeaderWords;
	IOToken_Input_Dscr_t        inTokenDscr;
	void                        *data_p, *data2_p;

	memset(&params, 0, sizeof(params));
	memset(&protocolParams, 0, sizeof(protocolParams));
	memset(&tokenParams, 0, sizeof(tokenParams));
	memset(&inTokenDscr, 0, sizeof(inTokenDscr));

	hashKeyBytes = (x_p->aalg->alg_key_len + 7) / 8;

	/***** Build SA *****/
	rc = SABuilder_Init_Basic(&params, &protocolParams, SAB_DIRECTION_OUTBOUND);
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][hash] SABuilder_Init_Basic failed, rc = %d\n", rc);
		goto err_rtn;
	}
	else
	{
		params.AuthAlgo = hashAlgoInfo_p->hashAlgo;
		/* Dummy authentication key must be supplied when SAB_FLAG_HASH_INTERMEDIATE is set */
		params.AuthKey1_p = mtk_crypto_dummyAuthKey;
		params.flags |= SAB_FLAG_HASH_SAVE | SAB_FLAG_HASH_INTERMEDIATE;
		params.flags |= SAB_FLAG_SUPPRESS_PAYLOAD;
		protocolParams.ICVByteCount = hashAlgoInfo_p->digestBytes;

		rc = SABuilder_GetSizes(&params, &(hashDigestDesc_p->SA_WordCount), NULL, NULL);
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][hash] SABuilder_GetSizes failed, rc = %d\n", rc);
			goto err_rtn;
		}
		else if (hashDigestDesc_p->SA_WordCount > MTK_CRYPTO_SA_WORD_MAX)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][hash] SA words %d > max words %d\n",
				hashDigestDesc_p->SA_WordCount, MTK_CRYPTO_SA_WORD_MAX);
			goto err_rtn;
		}
	}

	data_p = Adapter_DMAResource_HostAddr(
			Adapter_DMABuf_Handle2DMAResourceHandle(hashDigestDesc_p->saHandle));

	rc = SABuilder_BuildSA(&params, (uint32_t *)(data_p), NULL, NULL);
	if (rc == 0)
	{
		rc = PEC_SA_Register(interfaceId, hashDigestDesc_p->saHandle,
							 DMABuf_NULLHandle, DMABuf_NULLHandle);
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][hash] PEC_SA_Register failed, rc = %d\n", rc);
			goto err_rtn;
		}
	}
	else
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][hash] SABuilder_BuildSA failed, rc = %d\n", rc);
		goto err_rtn;
	}

	/***** Build In/OutDigest Data Buffer *****/
	hashDigestDesc_p->digestBytes = MAX(hashAlgoInfo_p->blockBytes,
										hashAlgoInfo_p->digestBytes);
	if (hashDigestDesc_p->digestBytes > MTK_CRYPTO_DIGEST_BYTE_MAX)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][hash] digest bytes %d > max bytes %d\n",
			hashDigestDesc_p->digestBytes, MTK_CRYPTO_DIGEST_BYTE_MAX);
		goto unregister_sa;
	}
	else
	{
		data_p = Adapter_DMAResource_HostAddr(
			Adapter_DMABuf_Handle2DMAResourceHandle(hashDigestDesc_p->inDigestHandle));

		data2_p = Adapter_DMAResource_HostAddr(
			Adapter_DMABuf_Handle2DMAResourceHandle(hashDigestDesc_p->outDigestHandle));

		words = hashKeyBytes / 4;
		for (idx = 0; idx < words; idx++)
		{
			((uint32_t*)(data_p))[idx] =
					((uint32_t*)(x_p->aalg->alg_key))[idx] ^ 0x36363636;
			((uint32_t*)(data2_p))[idx] =
					((uint32_t*)(x_p->aalg->alg_key))[idx] ^ 0x5c5c5c5c;

		}
		words = hashAlgoInfo_p->blockBytes / 4;
		for(; idx < words; idx++)
		{
			((uint32_t*)(data_p))[idx] = 0x36363636;
			((uint32_t*)(data2_p))[idx] = 0x5c5c5c5c;
		}
	}

	/***** Build Token *****/
	rc = TokenBuilder_BuildContext(&params, hashDigestDesc_p->tcr_arr);
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][hash] TokenBuilder_BuildContext failed, rc = %d\n", rc);
		goto unregister_sa;
	}
	else
	{
		rc = TokenBuilder_GetSize(hashDigestDesc_p->tcr_arr, &tokenMaxWords);
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][hash] TokenBuilder_GetSize failed, rc = %d\n", rc);
			goto unregister_sa;
		}
		else if (tokenMaxWords > MTK_CRYPTO_TOKEN_WORD_MAX)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][hash] token words %d > max words %d\n",
				tokenMaxWords, MTK_CRYPTO_TOKEN_WORD_MAX);
			goto unregister_sa;
		}
		else
		{
			tokenParams.PacketFlags |= TKB_PACKET_FLAG_HASHFIRST |TKB_PACKET_FLAG_HASHAPPEND;
			rc = TokenBuilder_BuildToken(hashDigestDesc_p->tcr_arr,
										 NULL, /* NOT USED */
										 hashAlgoInfo_p->blockBytes,
										 &tokenParams,
										 (uint32_t*)(hashDigestDesc_p->tokenBuf_p),
										 &(hashDigestDesc_p->Token_WordCount),
										 &tokenHeaderWords);
			if (rc != 0)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn][hash] TokenBuilder_BuildToken failed, rc = %d\n", rc);
				goto unregister_sa;
			}
		}
	}

	/*** Build InputToken (token header) ***/
	inTokenDscr.TknHdrWordInit = tokenHeaderWords;
	inTokenDscr.InPacket_ByteCount = hashAlgoInfo_p->blockBytes;
	inTokenDscr.Ext_p = NULL;
	inTokenDscr.AppID = 0;	/* per pkt assign */
	rc = IOToken_Create(&inTokenDscr, hashDigestDesc_p->inToken_arr);
	if (rc < 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][hash] IOToken_Create failed, rc = %d\n", rc);
		goto unregister_sa;
	}

	return 0;

unregister_sa:
	PEC_SA_UnRegister(interfaceId, hashDigestDesc_p->saHandle,
		              DMABuf_NULLHandle, DMABuf_NULLHandle);
err_rtn:
	return -1;
}

static int
mtk_crypto_buildEspDsec(
	struct xfrm_state              *x_p,
	uint32_t                       spi,
	uint32_t                       is_decrypt,
	uint32_t                       seq,
	mtk_crypto_cipherAlgoInfo_t    *cipherAlgoInfo_p,
	mtk_crypto_authAlgoInfo_t      *hashAlgoInfo_p,
	mtk_crypto_espDesc_t           *espDesc_p)
{
	int                         rc;
	TokenBuilder_Params_t       tokenParams;
	uint32_t                    tokenMaxWords=0, tokenHeaderWords;
	TokenBuilder_Context_t      *tokenContext_p;
	IOToken_Input_Dscr_t        inTokenDscr;
	uint32_t                    swap_spi;

	memset(&tokenParams, 0, sizeof(tokenParams));
	memset(&inTokenDscr, 0, sizeof(inTokenDscr));

	espDesc_p->is_decrypt = is_decrypt;

	/***** Init SA param (SA would be build after hash digest done) *****/
	swap_spi = ((((spi)>>24)&0xff) | (((spi)>>8)&0xff00) |
				(((spi)<<8)&0xff0000) | (((spi)<<24)&0xff000000));

	rc = SABuilder_Init_ESP(&(espDesc_p->params), &(espDesc_p->ipsecParams), swap_spi,
				((x_p->props.mode == XFRM_MODE_TUNNEL) ? SAB_IPSEC_TUNNEL : SAB_IPSEC_TRANSPORT),
				SAB_IPSEC_IPV4, /* unused parameter, always set to ipv4 */
				((is_decrypt) ? SAB_DIRECTION_INBOUND : SAB_DIRECTION_OUTBOUND));
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][%s] SABuilder_Init_Basic failed, rc = %d\n",
			((is_decrypt)?"dec":"enc"), rc);
		goto err_rtn;
	}
	else
	{
		espDesc_p->params.CryptoAlgo = cipherAlgoInfo_p->cryptoAlgo;
		espDesc_p->params.CryptoMode = cipherAlgoInfo_p->cryptoMode;
		if (cipherAlgoInfo_p->cryptoMode == SAB_CRYPTO_MODE_CTR)
		{
			espDesc_p->params.KeyByteCount = (x_p->ealg->alg_key_len + 7) / 8 - CTR_RFC3686_NONCE_SIZE;
			espDesc_p->params.Key_p = x_p->ealg->alg_key;
			espDesc_p->params.Nonce_p = ((uint8_t*)(x_p->ealg->alg_key)) + espDesc_p->params.KeyByteCount;
		}
		else
		{
			espDesc_p->params.KeyByteCount = (x_p->ealg->alg_key_len + 7) / 8;
			espDesc_p->params.Key_p = x_p->ealg->alg_key;
		}
		espDesc_p->params.AuthAlgo = hashAlgoInfo_p->hmacAlgo;
		/* AuthKey1/2_p would be reassigned after hash digest done */
		espDesc_p->params.AuthKey1_p = mtk_crypto_dummyAuthKey;
		espDesc_p->params.AuthKey2_p = mtk_crypto_dummyAuthKey;

		espDesc_p->ipsecParams.SeqNum = seq;

		rc = SABuilder_GetSizes(&(espDesc_p->params), &(espDesc_p->SA_WordCount),
								NULL, NULL);
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] SABuilder_GetSizes failed, rc = %d\n",
				((is_decrypt)?"dec":"enc"), rc);
			goto err_rtn;
		}
		else if (espDesc_p->SA_WordCount > MTK_CRYPTO_SA_WORD_MAX)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] SA words %d > max words %d\n",
				((is_decrypt)?"dec":"enc"), espDesc_p->SA_WordCount,
				MTK_CRYPTO_SA_WORD_MAX);
			goto err_rtn;
		}
	}

	/***** Build Token *****/
	rc = TokenBuilder_BuildContext(&(espDesc_p->params), espDesc_p->tcr_arr);
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][%s] TokenBuilder_BuildContext failed, rc = %d\n",
			((is_decrypt)?"dec":"enc"), rc);
		goto err_rtn;
	}
	else
	{
		rc = TokenBuilder_GetSize(espDesc_p->tcr_arr, &tokenMaxWords);
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] TokenBuilder_GetSize failed, rc = %d\n",
				((is_decrypt)?"dec":"enc"), rc);
			goto err_rtn;
		}
		else if (tokenMaxWords > MTK_CRYPTO_TOKEN_WORD_MAX)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] token words %d > max words %d\n",
				((is_decrypt)?"dec":"enc"), tokenMaxWords,
				MTK_CRYPTO_TOKEN_WORD_MAX);
			goto err_rtn;
		}
		else
		{
			tokenParams.PacketFlags = 0;
		    tokenParams.PadByte = 0x04;  /* next hdr, would be changed per packet */
		    tokenParams.IV_p = NULL;
		    tokenParams.BypassByteCount = 0;
		    tokenParams.AdditionalValue = 0;

			tokenContext_p = (TokenBuilder_Context_t*)(espDesc_p->tcr_arr);

			espDesc_p->pktTmpBytes = MTK_CRYPTO_PACKET_BYTE_TMP + 8 +
										tokenContext_p->IVByteCount +
										tokenContext_p->ICVByteCount;

			rc = TokenBuilder_BuildToken(espDesc_p->tcr_arr,
										 NULL, /* NOT USED */
										 espDesc_p->pktTmpBytes, /* per pkt assign */
										 &tokenParams,
										 (uint32_t*)(espDesc_p->tokenBuf_p),
										 &(espDesc_p->Token_WordCount),
										 &tokenHeaderWords);
			if (rc == 0)
			{
				espDesc_p->padBlockBytes = mtk_crypto_PadBlockSize(tokenContext_p->PadBlockByteCount,
													   tokenParams.AdditionalValue);

				if (tokenContext_p->protocol == TKB_PROTO_ESP_IN)
				{
					espDesc_p->dirTokenIdx = (tokenContext_p->IVHandling == 0) ? 2 : 1;
				}
				else if (tokenContext_p->protocol == TKB_PROTO_ESP_OUT)
				{
					espDesc_p->dirTokenIdx = (tokenContext_p->IVHandling == 2) ? 2 : 1;
					espDesc_p->insNxtPadTokenIdx = espDesc_p->dirTokenIdx + 1;
				}
				else
				{
					MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
									 "[mtk-crypto][warn][%s] not support protocol %d\n",
									 ((is_decrypt)?"dec":"enc"),
									 tokenContext_p->protocol);
					goto err_rtn;
				}
			}
			else
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn][%s] TokenBuilder_BuildToken failed, rc = %d\n",
					((is_decrypt)?"dec":"enc"), rc);
				goto err_rtn;
			}
		}
	}

	/*** Build InputToken (token header) ***/
	inTokenDscr.TknHdrWordInit = tokenHeaderWords;
	inTokenDscr.InPacket_ByteCount = espDesc_p->pktTmpBytes; /* per pkt assign */
	inTokenDscr.Ext_p = NULL;
	inTokenDscr.AppID = (is_decrypt) ? MTK_CRYPTO_APP_DEC : MTK_CRYPTO_APP_ENC;
	rc = IOToken_Create(&inTokenDscr, espDesc_p->inToken_arr);
	if (rc < 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn][%s] IOToken_Create failed, rc = %d\n",
			((is_decrypt)?"dec":"enc"), rc);
		goto err_rtn;
	}

	return 0;

err_rtn:
	return -1;
}

static int
mtk_crypto_sendReq(
	struct xfrm_state    *x_p,
	struct sk_buff       *skb_p,
	uint32_t             is_decrypt)
{
	int                      rc = 0;
	uint32_t                 notify_wq = 0, is_new_flow, do_linearize, seq;
	mtk_crypto_flowInfo_t    *flowInfo_p = NULL;
	struct sk_buff           *inDigest_skb_p = NULL, *outDigest_skb_p = NULL;

	if (atomic_read(&mtk_crypto_en) > 0)
	{
		rc = mtk_crypto_checkXfrmState(x_p);
		if (rc == 0)
		{
			rc = mtk_crypto_allocFlowInfo(x_p, is_decrypt, &flowInfo_p, &is_new_flow);
		}

		if (rc != 0)
		{
			return -EACCES;
		}
		else if (atomic_read(&mtk_crypto_eip97_en) <= 0)
		{
			rc = LKM_EIP97_Enable(1);
			if (rc == 0)
			{
				atomic_set(&mtk_crypto_eip97_en, 1);
			}
			else
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] enable eip97 fail\n");
				return -EACCES;
			}
		}
	}
	else
	{
		return -EACCES;
	}

	rc = mtk_crypto_checkSkb(x_p, skb_p, is_decrypt, &do_linearize);

	MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

	if ((flowInfo_p->tobe_del) ||
		(flowInfo_p->spi != x_p->id.spi))
	{
		MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);

		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] the flow is to be deleted (spi 0x%x)\n",
				((is_decrypt)?"dec":"enc"), x_p->id.spi);
		return -EPERM;
	}
	else
	{
		flowInfo_p->counter.inPktTot++;
		if (do_linearize)
		{
			flowInfo_p->counter.linearize++;
		}
	}

	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
		"[mtk-crypto][warn][%s] check skb fail (spi 0x%x), rc = %d\n",
		((is_decrypt)?"dec":"enc"), x_p->id.spi, rc);
		goto rtn_err_unlock;
	}
	else if (is_new_flow)
	{
		rc = mtk_crypto_buildHashDsec(x_p, flowInfo_p->hashAlgoInfo_p,
									  &(flowInfo_p->hashDigestDesc));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] build hash desc fail (spi 0x%x), rc = %d\n",
				((is_decrypt)?"dec":"enc"), x_p->id.spi, rc);
			goto rtn_err_unlock;
		}
		else
		{
			inDigest_skb_p = kzalloc(sizeof(struct sk_buff), GFP_ATOMIC);
			if (inDigest_skb_p == NULL)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn][%s] alloc inDigest skb fail (spi 0x%x)\n",
					((is_decrypt)?"dec":"enc"), x_p->id.spi);
			}
			else
			{
				outDigest_skb_p = kzalloc(sizeof(struct sk_buff), GFP_ATOMIC);
				if (outDigest_skb_p == NULL)
				{
					MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
						"[mtk-crypto][warn][%s] alloc outDigest skb fail (spi 0x%x)\n",
						((is_decrypt)?"dec":"enc"), x_p->id.spi);
					goto free_digest_skb;
				}
			}
		}

		seq = (is_decrypt) ?
			  (htonl(MTK_CRYPTO_SKB_CB(skb_p)->xfrm.seq.input.low)) :
			  (htonl(MTK_CRYPTO_SKB_CB(skb_p)->xfrm.seq.output.low));

		rc = mtk_crypto_buildEspDsec(x_p, x_p->id.spi, is_decrypt, seq,
									 flowInfo_p->cipherAlgoInfo_p,
									 flowInfo_p->hashAlgoInfo_p,
									 &(flowInfo_p->espDesc));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn][%s] build esp desc fail (spi 0x%x), rc = %d\n",
				((is_decrypt)?"dec":"enc"), x_p->id.spi, rc);
			goto free_digest_skb;
		}

		MTK_CRYPTO_SKB_CB(skb_p)->flowInfo_p = flowInfo_p;
		MTK_CRYPTO_SKB_CB(inDigest_skb_p)->flowInfo_p = flowInfo_p;
		MTK_CRYPTO_SKB_CB(outDigest_skb_p)->flowInfo_p = flowInfo_p;

		skb_queue_tail(&flowInfo_p->skbQueue, skb_p);

		skb_queue_tail(&mtk_crypto_skbQueue, inDigest_skb_p);
		skb_queue_tail(&mtk_crypto_skbQueue, outDigest_skb_p);
		flowInfo_p->counter.queuePutTot += 2;
		notify_wq = 1;

		MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);
	}
	else
	{
		MTK_CRYPTO_SKB_CB(skb_p)->flowInfo_p = flowInfo_p;

		if (flowInfo_p->digestDone_bmp == 0x3)
		{
			skb_queue_tail(&mtk_crypto_skbQueue, skb_p);
			flowInfo_p->counter.queuePutTot++;
			notify_wq = 1;

			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
							 "[mtk-crypto][info] Put packet to skb_queue\n");
		}
		else
		{
			skb_queue_tail(&flowInfo_p->skbQueue, skb_p);
		}

		MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);
	}

	if (notify_wq)
	{
		queue_work(mtk_crypto_ring0_wq_p, &mtk_crypto_ring0PutPkt_work);
	}

	return -EINPROGRESS;

free_digest_skb:
	if (NULL != inDigest_skb_p)
	{
		kfree(inDigest_skb_p);
	}
	if (NULL != outDigest_skb_p)
	{
		kfree(outDigest_skb_p);
	}
rtn_err_unlock:
	flowInfo_p->counter.inPktDrop++;
	MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);
	return -EPERM;
}

static void
mtk_crypto_deleteFlow(
	uint32_t spi)
{
	int                     idx;
	mtk_crypto_flowInfo_t   *flowInfo_p;
	mtk_crypto_flowInfo_t   *flowInfo_arr;
	spinlock_t              *lock_p;

	lock_p = &mtk_crypto_decFlow_lock;
	flowInfo_arr = mtk_crypto_decFlow_arr;

	MTK_CRYPTO_SPIN_LOCK(lock_p);
	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if ((flowInfo_arr[idx].x_p != NULL) &&
			(flowInfo_arr[idx].spi == spi))
		{
			flowInfo_p = &flowInfo_arr[idx];

			MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

			flowInfo_p->tobe_del = 1;

			if (flowInfo_p->counter.ringGetTot + flowInfo_p->counter.queuePutDrop
				== flowInfo_p->counter.queuePutTot)
			{
				mtk_crypto_clearFlowDb(flowInfo_p);
			}

			MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);

			break;
		}
	}
	MTK_CRYPTO_SPIN_UNLOCK(lock_p);


	if (idx >= MTK_CRYPTO_FLOW_NUM)
	{
		lock_p = &mtk_crypto_encFlow_lock;
		flowInfo_arr = mtk_crypto_encFlow_arr;

		MTK_CRYPTO_SPIN_LOCK(lock_p);
		for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
		{
			if ((flowInfo_arr[idx].x_p != NULL) &&
				(flowInfo_arr[idx].spi == spi))
			{
				flowInfo_p = &flowInfo_arr[idx];

				MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

				flowInfo_p->tobe_del = 1;

				if (flowInfo_p->counter.ringGetTot + flowInfo_p->counter.queuePutDrop
					== flowInfo_p->counter.queuePutTot)
				{
					mtk_crypto_clearFlowDb(flowInfo_p);
				}

				MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);

				break;
			}
		}
		MTK_CRYPTO_SPIN_UNLOCK(lock_p);
	}
}

static void
mtk_crypto_deleteAllFlow(
	uint32_t is_decrypt)
{
	int                     idx;
	mtk_crypto_flowInfo_t   *flowInfo_p;
	mtk_crypto_flowInfo_t   *flowInfo_arr;
	spinlock_t              *lock_p;

	if (is_decrypt)
	{
		lock_p = &mtk_crypto_decFlow_lock;
		flowInfo_arr = mtk_crypto_decFlow_arr;
	}
	else
	{
		lock_p = &mtk_crypto_encFlow_lock;
		flowInfo_arr = mtk_crypto_encFlow_arr;
	}

	MTK_CRYPTO_SPIN_LOCK(lock_p);
	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if (flowInfo_arr[idx].x_p != NULL)
		{
			flowInfo_p = &flowInfo_arr[idx];

			MTK_CRYPTO_SPIN_LOCK(&flowInfo_p->lock);

			flowInfo_p->tobe_del = 1;

			if (flowInfo_p->counter.ringGetTot + flowInfo_p->counter.queuePutDrop
				== flowInfo_p->counter.queuePutTot)
			{
				mtk_crypto_clearFlowDb(flowInfo_p);
			}

			MTK_CRYPTO_SPIN_UNLOCK(&flowInfo_p->lock);
		}
	}
	MTK_CRYPTO_SPIN_UNLOCK(lock_p);
}

static int
mtk_crypto_enEip97_procRead(
	struct seq_file    *seq_p,
	void               *v_p)
{
	seq_printf(seq_p, "eip97-enable=%d\n", (atomic_read(&mtk_crypto_en) > 0) ? 1 : 0);
	return 0;
}

static int
mtk_crypto_enEip97_procOpen(
	struct inode    *inode_p,
	struct file     *file_p)
{
	return single_open(file_p, mtk_crypto_enEip97_procRead, NULL);
}

static ssize_t
mtk_crypto_enEip97_procWrite(
	struct file          *file,
	const char __user    *buffer,
	size_t               count,
	loff_t               *data)
{
	char   c;

	if (count > 0)
	{
		if (get_user(c, buffer))
		{
			return -EFAULT;
		}

		if (c == '0')
		{
			if (atomic_read(&mtk_crypto_en) > 0)
			{
				atomic_set(&mtk_crypto_en , 0);

				mtk_crypto_deleteAllFlow(1);
				mtk_crypto_deleteAllFlow(0);

				// need to wait all pkt sent out from eip97
				//LKM_EIP97_Enable(0);
			}
		}
		else
		{
			if (atomic_read(&mtk_crypto_en) == 0)
			{
				atomic_set(&mtk_crypto_en , 1);

				//LKM_EIP97_Enable(1);
			}
		}
	}

	return count;
}

static int
mtk_crypto_enLog_procRead(
	struct seq_file    *seq_p,
	void               *v_p)
{
	if (mtk_crypto_logLevel == 0)
	{
		seq_printf(seq_p, "log-level=%d (%s)\n", mtk_crypto_logLevel, "disable");
	}
	else if (mtk_crypto_logLevel == MTK_CRYPTO_LOG_ERR)
	{
		seq_printf(seq_p, "log-level=%d (%s)\n", mtk_crypto_logLevel, "err");
	}
	else if (mtk_crypto_logLevel == MTK_CRYPTO_LOG_WARN)
	{
		seq_printf(seq_p, "log-level=%d (%s)\n", mtk_crypto_logLevel, "warn");
	}
	else
	{
		seq_printf(seq_p, "log-level=%d (%s)\n", mtk_crypto_logLevel, "info");
	}

	return 0;
}

static int
mtk_crypto_enLog_procOpen(
	struct inode    *inode_p,
	struct file     *file_p)
{
	return single_open(file_p, mtk_crypto_enLog_procRead, NULL);
}

static ssize_t
mtk_crypto_enLog_procWrite(
	struct file          *file,
	const char __user    *buffer,
	size_t               count,
	loff_t               *data)
{
	char   c;
	int    val;

	if (count > 0)
	{
		if (get_user(c, buffer))
		{
			return -EFAULT;
		}

		val = c - '0';
		mtk_crypto_logLevel = (val >= 0) ? val : 0;
	}

	return count;
}

static int
mtk_crypto_dbg_procRead(
	struct seq_file    *seq_p,
	void               *v_p)
{
	int idx;

	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if (mtk_crypto_decFlow_arr[idx].x_p != NULL)
		{
			seq_printf(seq_p, "\n[Decrypt Flow %d (spi=0x%x)]\n",
					   idx, mtk_crypto_decFlow_arr[idx].spi);

			seq_printf(seq_p, "-- inPktTot=%d, inPktDrop=%d\n",
					   mtk_crypto_decFlow_arr[idx].counter.inPktTot,
					   mtk_crypto_decFlow_arr[idx].counter.inPktDrop);
			seq_printf(seq_p, "-- QueueTot=%d, queuePutDrop=%d\n",
					   mtk_crypto_decFlow_arr[idx].counter.queuePutTot,
					   mtk_crypto_decFlow_arr[idx].counter.queuePutDrop);
			seq_printf(seq_p, "-- ringGetTot=%d, ringGetAppFail=%d, ringGetSeqFail=%d, ringGetOtherFail=%d\n",
					   mtk_crypto_decFlow_arr[idx].counter.ringGetTot,
					   mtk_crypto_decFlow_arr[idx].counter.ringGetAppFail,
					   mtk_crypto_decFlow_arr[idx].counter.ringGetSeqFail,
					   mtk_crypto_decFlow_arr[idx].counter.ringGetOtherFail);

			seq_printf(seq_p, "-- expandCopy=%d, linearize=%d\n",
					   mtk_crypto_decFlow_arr[idx].counter.expandCopy,
					   mtk_crypto_decFlow_arr[idx].counter.linearize);
		}
	}

	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if (mtk_crypto_encFlow_arr[idx].x_p != NULL)
		{
			seq_printf(seq_p, "\n[Encrypt Flow %d (spi=0x%x)]\n",
					   idx, mtk_crypto_encFlow_arr[idx].spi);

			seq_printf(seq_p, "-- inPktTot=%d, inPktDrop=%d\n",
					   mtk_crypto_encFlow_arr[idx].counter.inPktTot,
					   mtk_crypto_encFlow_arr[idx].counter.inPktDrop);
			seq_printf(seq_p, "-- QueueTot=%d, queuePutDrop=%d\n",
					   mtk_crypto_encFlow_arr[idx].counter.queuePutTot,
					   mtk_crypto_encFlow_arr[idx].counter.queuePutDrop);
			seq_printf(seq_p, "-- ringGetTot=%d, ringGetAppFail=%d, ringGetSeqFail=%d, ringGetOtherFail=%d\n",
					   mtk_crypto_encFlow_arr[idx].counter.ringGetTot,
					   mtk_crypto_encFlow_arr[idx].counter.ringGetAppFail,
					   mtk_crypto_encFlow_arr[idx].counter.ringGetSeqFail,
					   mtk_crypto_encFlow_arr[idx].counter.ringGetOtherFail);

			seq_printf(seq_p, "-- expandCopy=%d, linearize=%d\n",
					   mtk_crypto_encFlow_arr[idx].counter.expandCopy,
					   mtk_crypto_encFlow_arr[idx].counter.linearize);
		}
	}

	return 0;
}

static int
mtk_crypto_dbg_procOpen(
	struct inode    *inode_p,
	struct file     *file_p)
{
	return single_open(file_p, mtk_crypto_dbg_procRead, NULL);
}

static const struct file_operations mtk_crypto_enEip97_procFop = {
	.owner      = THIS_MODULE,
	.open       = mtk_crypto_enEip97_procOpen,
	.read       = seq_read,
	.write      = mtk_crypto_enEip97_procWrite,
	.llseek     = seq_lseek,
	.release    = single_release
};

static const struct file_operations mtk_crypto_enLog_procFop = {
	.owner      = THIS_MODULE,
	.open       = mtk_crypto_enLog_procOpen,
	.read       = seq_read,
	.write      = mtk_crypto_enLog_procWrite,
	.llseek     = seq_lseek,
	.release    = single_release
};

static const struct file_operations mtk_crypto_dbg_procFop = {
	.owner      = THIS_MODULE,
	.open       = mtk_crypto_dbg_procOpen,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release
};

static int
mtk_crypto_initProc(void)
{
	if (NULL == mtk_crypto_procDir)
	{
		mtk_crypto_procDir = proc_mkdir("mtk-crypto", NULL);
	}

	mtk_crypto_enEip97_procDir = proc_create("en-eip97", 0, mtk_crypto_procDir,
											 &mtk_crypto_enEip97_procFop);
	if (NULL == mtk_crypto_enEip97_procDir)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create %s proc fail\n", "en-eip97");
		goto err_rtn;
	}

	mtk_crypto_enLog_procDir = proc_create("en-log", 0, mtk_crypto_procDir,
										   &mtk_crypto_enLog_procFop);
	if (NULL == mtk_crypto_enLog_procDir)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create %s proc fail\n", "en-log");
		goto err_rtn;
	}

	mtk_crypto_dbg_procDir = proc_create("dbg", 0, mtk_crypto_procDir,
										 &mtk_crypto_dbg_procFop);
	if (NULL == mtk_crypto_dbg_procDir)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create %s proc fail\n", "dbg");
		goto err_rtn;
	}

	return 0;

err_rtn:
	return -1;
}

static void
mtk_crypto_deinitProc(void)
{
	if (NULL != mtk_crypto_procDir)
	{
		if (NULL != mtk_crypto_enEip97_procDir)
		{
			remove_proc_entry("en-eip97", mtk_crypto_procDir);
		}
		if (NULL != mtk_crypto_enLog_procDir)
		{
			remove_proc_entry("en-log", mtk_crypto_procDir);
		}
		if (NULL != mtk_crypto_dbg_procDir)
		{
			remove_proc_entry("dbg", mtk_crypto_procDir);
		}
		remove_proc_entry("mtk-crypto", NULL);
	}
}
static int
mtk_crypto_initFlowArr(
	uint32_t is_decrypt)
{
	int                      rc;
	uint32_t                 idx;
	DMABuf_Properties_t      DMAProperties = {false, MTK_CRYPTO_DMA_ALIGN_BYTE, 0, 0};
	void                     *data_p;
	dma_addr_t               dma_addr;
	mtk_crypto_flowInfo_t    *flowInfo_arr;

	if (is_decrypt)
	{
		memset(mtk_crypto_decFlow_arr, 0, sizeof(mtk_crypto_decFlow_arr));
		flowInfo_arr = mtk_crypto_decFlow_arr;
	}
	else
	{
		memset(mtk_crypto_encFlow_arr, 0, sizeof(mtk_crypto_encFlow_arr));
		flowInfo_arr = mtk_crypto_encFlow_arr;
	}

	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		spin_lock_init(&(flowInfo_arr[idx].lock));
		skb_queue_head_init(&flowInfo_arr[idx].skbQueue);

		/* init hashDigestDesc */
		flowInfo_arr[idx].hashDigestDesc.tokenBuf_p =
						kzalloc(MTK_CRYPTO_TOKEN_WORD_MAX * 4, GFP_KERNEL);
		if (NULL == flowInfo_arr[idx].hashDigestDesc.tokenBuf_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d hash TokenBuf fail\n", idx);
			goto err_rtn;
		}

		data_p = dma_pool_alloc(mtk_crypto_sa_pool, GFP_KERNEL, &dma_addr);
		if (NULL == data_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d hash SA fail\n", idx);
			goto err_rtn;

		}
		DMAProperties.Size = MTK_CRYPTO_SA_WORD_MAX * 4;
		rc = DMABuf_Register(DMAProperties, data_p, (void *)(dma_addr), 'C',
					&(flowInfo_arr[idx].hashDigestDesc.saHandle));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] register flow %d hash SA fail\n", idx);
				goto err_rtn;
		}

		data_p = dma_pool_alloc(mtk_crypto_digest_pool, GFP_KERNEL, &dma_addr);
		if (NULL == data_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d hash InDigest data fail\n", idx);
			goto err_rtn;

		}
		DMAProperties.Size = MTK_CRYPTO_DIGEST_BYTE_MAX;
		rc = DMABuf_Register(DMAProperties, data_p, (void *)(dma_addr), 'C',
					&(flowInfo_arr[idx].hashDigestDesc.inDigestHandle));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] register flow %d hash InDigest data fail\n", idx);
			goto err_rtn;
		}

		data_p = dma_pool_alloc(mtk_crypto_digest_pool, GFP_KERNEL, &dma_addr);
		if (NULL == data_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d hash OutDigest data fail\n", idx);
			goto err_rtn;

		}
		DMAProperties.Size = MTK_CRYPTO_DIGEST_BYTE_MAX;
		rc = DMABuf_Register(DMAProperties, data_p, (void *)(dma_addr), 'C',
					&(flowInfo_arr[idx].hashDigestDesc.outDigestHandle));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] register flow %d hash OutDigest data fail\n", idx);
			goto err_rtn;
		}

		/* init espDesc */
		flowInfo_arr[idx].espDesc.tokenBuf_p =
						kzalloc(MTK_CRYPTO_TOKEN_WORD_MAX * 4, GFP_KERNEL);
		if (NULL == flowInfo_arr[idx].espDesc.tokenBuf_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d esp TokenBuf fail\n", idx);
			goto err_rtn;
		}

		data_p = dma_pool_alloc(mtk_crypto_sa_pool, GFP_KERNEL, &dma_addr);
		if (NULL == data_p)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] alloc flow %d esp SA fail\n", idx);
			goto err_rtn;

		}
		DMAProperties.Size = MTK_CRYPTO_SA_WORD_MAX * 4;
		rc = DMABuf_Register(DMAProperties, data_p, (void *)(dma_addr), 'C',
					&(flowInfo_arr[idx].espDesc.saHandle));
		if (rc != 0)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] register flow %d esp SA fail\n", idx);
			goto err_rtn;
		}
	}

	return 0;

err_rtn:
	return -1;
}

static void
mtk_crypto_deinitFlowArr(
	uint32_t is_decrypt)
{
	uint32_t                 idx;
	mtk_crypto_flowInfo_t    *flowInfo_arr;

	if (is_decrypt)
	{
		flowInfo_arr = mtk_crypto_decFlow_arr;
	}
	else
	{
		flowInfo_arr = mtk_crypto_encFlow_arr;
	}

	for (idx = 0; idx < MTK_CRYPTO_FLOW_NUM; idx++)
	{
		if (NULL != flowInfo_arr[idx].hashDigestDesc.tokenBuf_p)
		{
			kfree(flowInfo_arr[idx].hashDigestDesc.tokenBuf_p);
		}
		if (NULL != flowInfo_arr[idx].espDesc.tokenBuf_p)
		{
			kfree(flowInfo_arr[idx].espDesc.tokenBuf_p);
		}
	}
}

int
mtk_crypto_init(void)
{
	int                    rc;
	PEC_InitBlock_t        pec_initBlock = {0, 0};
	uint32_t               pec_idx, idx, interfaceId = 0; /* ring 0 */
	DMABuf_Properties_t    DMAProperties = {false, MTK_CRYPTO_DMA_ALIGN_BYTE, 0, 0};
	dma_addr_t             dma_addr;
	void                   *data_p;

	rc = Driver97_Init();
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] Driver97_Init fail, rc=%d\n", rc);
		goto err_rtn;
	}

	/* ADAPTER_PEC_DEVICE_COUNT --> ring cnt */
	for (pec_idx = 0; pec_idx < ADAPTER_PEC_DEVICE_COUNT; pec_idx++)
	{
		/* pec_initBlock is unused param*/
		rc = PEC_Init(pec_idx, &pec_initBlock);
		if (rc != PEC_STATUS_OK)
		{
			MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
				"[mtk-crypto][warn] PEC %d init fail, rc=%d\n", pec_idx, rc);
			goto exit_pec;
		}
	}

	INIT_WORK(&mtk_crypto_ring0PutPkt_work, mtk_crypto_putRing0Pkt);
	mtk_crypto_ring0_wq_p = create_singlethread_workqueue(MTK_CRYPTO_RING0_WQ_NAME);

	if (NULL == mtk_crypto_ring0_wq_p)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create %s workqueue fail\n", MTK_CRYPTO_RING0_WQ_NAME);
		goto exit_pec;
	}

	mtk_crypto_sa_pool =
		dma_pool_create("mtk-crypto-sa-pool", Device_GetReference(NULL, NULL),
						MTK_CRYPTO_SA_WORD_MAX * 4, MTK_CRYPTO_DMA_ALIGN_BYTE, 0);
	if (NULL == mtk_crypto_sa_pool)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create sa dma-pool fail\n");
		goto exit_wq;
	}

	mtk_crypto_digest_pool =
		dma_pool_create("mtk-crypto-digest_pool", Device_GetReference(NULL, NULL),
						MTK_CRYPTO_DIGEST_BYTE_MAX, MTK_CRYPTO_DMA_ALIGN_BYTE, 0);
	if (NULL == mtk_crypto_digest_pool)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create digest dma-pool fail\n");
		goto exit_sa_pool;
	}

	mtk_crypto_token_pool =
		dma_pool_create("mtk-crypto-token-pool", Device_GetReference(NULL, NULL),
						MTK_CRYPTO_TOKEN_WORD_MAX * 4, MTK_CRYPTO_DMA_ALIGN_BYTE, 0);
	if (NULL == mtk_crypto_token_pool)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] create token dma-pool fail\n");
		goto exit_digest_pool;
	}

	mtk_crypto_cmdDesc_idx = 0;
	mtk_crypto_cmdDesc_arr =
		kzalloc(sizeof(PEC_CommandDescriptor_t) * DRIVER_PEC_MAX_PACKETS, GFP_KERNEL);
	if (NULL == mtk_crypto_cmdDesc_arr)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] alloc cmdDesc arr fail\n");
		goto exit_token_pool;
	}
	else
	{
		DMAProperties.Size = MTK_CRYPTO_TOKEN_WORD_MAX * 4;

		for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
		{
			mtk_crypto_cmdDesc_arr[idx].InputToken_p =
								kzalloc(IOTOKEN_IN_WORD_COUNT * 4, GFP_KERNEL);
			if (NULL == mtk_crypto_cmdDesc_arr[idx].InputToken_p)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] alloc cmdDesc %d InputToken fail\n", idx);
				goto exit_cmdDesc;
			}

			data_p = dma_pool_alloc(mtk_crypto_token_pool, GFP_KERNEL, &dma_addr);
			if (NULL == data_p)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] alloc cmdDesc %d token fail\n", idx);
				goto exit_cmdDesc;
			}

			rc = DMABuf_Register(DMAProperties, data_p, (void *)(dma_addr), 'C',
								 &(mtk_crypto_cmdDesc_arr[idx].Token_Handle));
			if (rc != 0)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] register cmdDesc %d Token_Handle fail\n", idx);
				goto exit_cmdDesc;
			}
		}
	}

	mtk_crypto_rsltDesc_arr =
		kzalloc(sizeof(PEC_ResultDescriptor_t) * DRIVER_PEC_MAX_PACKETS, GFP_KERNEL);
	if (NULL == mtk_crypto_rsltDesc_arr)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] alloc rsltDesc arr fail\n");
		goto exit_cmdDesc;
	}
	else
	{
		for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
		{
			mtk_crypto_rsltDesc_arr[idx].OutputToken_p =
								kzalloc(IOTOKEN_OUT_WORD_COUNT * 4, GFP_KERNEL);
			if (NULL == mtk_crypto_rsltDesc_arr[idx].OutputToken_p)
			{
				MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
					"[mtk-crypto][warn] alloc rsltDesc %d OutputToken fail\n", idx);
				goto exit_rsltDesc;
			}
		}
	}

	skb_queue_head_init(&mtk_crypto_skbQueue);
	spin_lock_init(&mtk_crypto_decFlow_lock);
	spin_lock_init(&mtk_crypto_encFlow_lock);

	rc = mtk_crypto_initFlowArr(1);
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] init dec flow arr fail\n");
		goto exit_decflowArr;
	}

	rc = mtk_crypto_initFlowArr(0);
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] init enc flow arr fail\n");
		goto exit_decflowArr;
	}

	rc = PEC_ResultNotify_Request(interfaceId, mtk_crypto_getRing0Pkt, 1);
	if (rc != PEC_STATUS_OK)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] request PEC ring %d result notify fail\n", interfaceId);
		goto exit_encflowArr;
	}

	rc = mtk_crypto_initProc();
	if (rc != 0)
	{
		MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_WARN,
			"[mtk-crypto][warn] init proc fail\n");
		goto exit_proc;
	}

	atomic_set(&mtk_crypto_en, 1);

#if IS_REACHABLE(CONFIG_INET_ESP)
	crypto_esp_sendReq_func_p = mtk_crypto_sendReq;
	MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO, "[mtk-crypto][info] register crypto_esp_sendReq_func_p\n");
#endif

//#if IS_REACHABLE(CONFIG_INET6_ESP)
//	crypto_esp6_sendReq_func_p = mtk_crypto_sendReq;
//	MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO, "[mtk-crypto][info] register crypto_esp6_sendReq_func_p\n");
//#endif
	crypto_deleteFlow_func_p = mtk_crypto_deleteFlow;
	MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO, "[mtk-crypto][info] register crypto_deleteFlow_func_p\n");

	MTK_CRYPTO_PRINT(MTK_CRYPTO_LOG_INFO,
					 "[mtk-crypto][info] driver init success\n");
	return 0;

exit_proc:
	mtk_crypto_deinitProc();
exit_encflowArr:
	mtk_crypto_deinitFlowArr(0);
exit_decflowArr:
	mtk_crypto_deinitFlowArr(1);
exit_rsltDesc:
	for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
	{
		if (NULL != mtk_crypto_rsltDesc_arr[idx].OutputToken_p)
		{
			kfree(mtk_crypto_rsltDesc_arr[idx].OutputToken_p);
		}
	}
	kfree(mtk_crypto_rsltDesc_arr);
exit_cmdDesc:
	for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
	{
		if (NULL != mtk_crypto_cmdDesc_arr[idx].InputToken_p)
		{
			kfree(mtk_crypto_cmdDesc_arr[idx].InputToken_p);
		}
	}
	kfree(mtk_crypto_cmdDesc_arr);
exit_token_pool:
	dma_pool_destroy(mtk_crypto_token_pool);
exit_digest_pool:
	dma_pool_destroy(mtk_crypto_digest_pool);
exit_sa_pool:
	dma_pool_destroy(mtk_crypto_sa_pool);
exit_wq:
	destroy_workqueue(mtk_crypto_ring0_wq_p);
exit_pec:
	for (idx = 0; idx < pec_idx; idx++)
	{
		PEC_UnInit(idx);
	}
	Driver97_Exit();
err_rtn:
	return -1;
}

void
mtk_crypto_exit(void)
{
	uint32_t    idx;

	atomic_set(&mtk_crypto_en, 0);

#if IS_REACHABLE(CONFIG_INET_ESP)
	crypto_esp_sendReq_func_p = NULL;
#endif

//#if IS_REACHABLE(CONFIG_INET6_ESP)
//	crypto_esp6_sendReq_func_p = NULL;
//#endif

	crypto_deleteFlow_func_p = NULL;

	mtk_crypto_deinitProc();

	LKM_EIP97_Enable(0);
	atomic_set(&mtk_crypto_eip97_en, 0);

	destroy_workqueue(mtk_crypto_ring0_wq_p);

	for (idx = 0; idx < ADAPTER_PEC_DEVICE_COUNT; idx++)
	{
		PEC_UnInit(idx);
	}
	Driver97_Exit();

	dma_pool_destroy(mtk_crypto_sa_pool);
	dma_pool_destroy(mtk_crypto_digest_pool);
	dma_pool_destroy(mtk_crypto_token_pool);

	for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
	{
		if (NULL != mtk_crypto_cmdDesc_arr[idx].InputToken_p)
		{
			kfree(mtk_crypto_cmdDesc_arr[idx].InputToken_p);
		}
	}
	kfree(mtk_crypto_cmdDesc_arr);

	for (idx = 0; idx < DRIVER_PEC_MAX_PACKETS; idx++)
	{
		if (NULL != mtk_crypto_rsltDesc_arr[idx].OutputToken_p)
		{
			kfree(mtk_crypto_rsltDesc_arr[idx].OutputToken_p);
		}
	}
	kfree(mtk_crypto_rsltDesc_arr);

	mtk_crypto_deinitFlowArr(1);
	mtk_crypto_deinitFlowArr(0);
}

MODULE_LICENSE(ADAPTER_LICENSE);

module_init(mtk_crypto_init);
module_exit(mtk_crypto_exit);

