

while read line; do
	key=$(echo ${line} | cut -f1 -d':')
	value=$(echo ${line} | cut -f2 -d':' | tr -d ' ')

	case "${key}" in
	"UUID")
		UUID=${value}
		;;
	"Data blocks")
		DATA_BLOCKS=${value}
		;;
	"Data block size")
		DATA_BLOCK_SIZE=${value}
		;;
	"Hash block size")
		HASH_BLOCK_SIZE=${value}
		;;
	"Hash algorithm")
		HASH_ALG=${value}
		;;
	"Salt")
		SALT=${value}
		;;
	"Root hash")
		ROOT_HASH=${value}
		;;
	esac
done

SECTORS=$((${DATA_BLOCKS} * 8))

# keyword "<rootfs_dev_path>" will be replaced in LK
pattern='dm-mod.create="a,,,ro, 0 '${SECTORS}' verity 1 <rootfs_dev_path> <rootfs_dev_path> '${DATA_BLOCK_SIZE}' '${HASH_BLOCK_SIZE}' '${DATA_BLOCKS}' '$((${DATA_BLOCKS} + 1))' sha256-generic '${ROOT_HASH}' '${SALT}'"'

echo write cmdline:$pattern to file:$1
echo $pattern > $1

