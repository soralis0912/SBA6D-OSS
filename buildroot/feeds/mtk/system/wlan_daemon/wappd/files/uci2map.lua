#!/usr/bin/lua
local uci2map = {}

local shuci = require("shuci")
function uci_load(filename)
      return shuci.decode(filename)
end

local uciCfgfile = "/etc/config/mapd"

function uci2map.__trim(s)
  if s then return (s:gsub("^%s*(.-)%s*$", "%1")) end
end

function uci2map.read_pipe(pipe)
    local retry_count = 10
    local fp, txt, err
    repeat  -- fp:read() may return error, "Interrupted system call", and can be recovered by doing it again
        fp = io.popen(pipe)
        txt, err = fp:read("*a")
        fp:close()
        retry_count = retry_count - 1
    until err == nil or retry_count == 0
    return txt
end

function uci_apply_mapd_configuration()

-- converting dpp_cfg file

	local file_name = "/etc/dpp_cfg.txt"
        local file
	local uci_cfg
	uci_cfg = uci_load(uciCfgfile)
	for _, uci_vif in pairs(uci_cfg["dpp_cfg"]) do
		os.execute("sed -i '/allowed_role=/d' "..file_name)
		os.execute("sed -i '/presence_band_priority=/d' "..file_name)
	        file = io.open(file_name, "a+")
		io.output(file)
		if uci_vif.presence_band_priority ~= nil and uci_vif.presence_band_priority ~= ' ' then
	              io.write("presence_band_priority=", uci_vif.presence_band_priority, "\n")
		end
	      	if uci_vif.allowed_role ~= nil then
	              io.write("allowed_role=", uci_vif.allowed_role, "\n")
		end
		io.close()
	end

-- converting mapd_strng file

	local file_name = "/tmp/mapd_strng.txt"
        local file
	for _, uci_vif in pairs(uci_cfg["mapd_strng"]) do
	        file = io.open(file_name, "w+")
		io.output(file)
	      	if uci_vif.LowRSSIAPSteerEdge_RE ~= nil then
	              io.write("LowRSSIAPSteerEdge_RE=", uci_vif.LowRSSIAPSteerEdge_RE, "\n")
		end
	      	if uci_vif.CUOverloadTh_2G ~= nil then
	              io.write("CUOverloadTh_2G=", uci_vif.CUOverloadTh_2G, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_L ~= nil then
	              io.write("CUOverloadTh_5G_L=", uci_vif.CUOverloadTh_5G_L, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_H ~= nil then
	              io.write("CUOverloadTh_5G_H=", uci_vif.CUOverloadTh_5G_H, "\n")
		end
	      	if uci_vif.CUOverloadTh_6G ~= nil then
	              io.write("CUOverloadTh_6G=", uci_vif.CUOverloadTh_6G, "\n")
		end
	      	if uci_vif.MetricPolicyChUtilThres_24G ~= nil then
	              io.write("MetricPolicyChUtilThres_24G=", uci_vif.MetricPolicyChUtilThres_24G, "\n")
		end
	      	if uci_vif.MetricPolicyChUtilThres_5GL ~= nil then
	              io.write("MetricPolicyChUtilThres_5GL=", uci_vif.MetricPolicyChUtilThres_5GL, "\n")
		end
	      	if uci_vif.MetricPolicyChUtilThres_5GH ~= nil then
	              io.write("MetricPolicyChUtilThres_5GH=", uci_vif.MetricPolicyChUtilThres_5GH, "\n")
		end
	      	if uci_vif.MetricPolicyChUtilThres_6G ~= nil then
	              io.write("MetricPolicyChUtilThres_6G=", uci_vif.MetricPolicyChUtilThres_6G, "\n")
		end
	      	if uci_vif.ChPlanningChUtilThresh_24G ~= nil then
	              io.write("ChPlanningChUtilThresh_24G=", uci_vif.ChPlanningChUtilThresh_24G, "\n")
		end
	      	if uci_vif.ChPlanningChUtilThresh_5GL ~= nil then
	              io.write("ChPlanningChUtilThresh_5GL=", uci_vif.ChPlanningChUtilThresh_5GL, "\n")
		end
	      	if uci_vif.ChPlanningChUtilThresh_6G ~= nil then
	              io.write("ChPlanningChUtilThresh_6G=", uci_vif.ChPlanningChUtilThresh_6G, "\n")
		end
	      	if uci_vif.ChPlanningEDCCAThresh_24G ~= nil then
	              io.write("ChPlanningEDCCAThresh_24G=", uci_vif.ChPlanningEDCCAThresh_24G, "\n")
		end
	      	if uci_vif.ChPlanningEDCCAThresh_5GL ~= nil then
	              io.write("ChPlanningEDCCAThresh_5GL=", uci_vif.ChPlanningEDCCAThresh_5GL, "\n")
		end
	      	if uci_vif.ChPlanningEDCCAThresh_6G ~= nil then
	              io.write("ChPlanningEDCCAThresh_6G=", uci_vif.ChPlanningEDCCAThresh_6G, "\n")
		end
	      	if uci_vif.ChPlanningOBSSThresh_24G ~= nil then
	              io.write("ChPlanningOBSSThresh_24G=", uci_vif.ChPlanningOBSSThresh_24G, "\n")
		end
	      	if uci_vif.ChPlanningOBSSThresh_5GL ~= nil then
	              io.write("ChPlanningOBSSThresh_5GL=", uci_vif.ChPlanningOBSSThresh_5GL, "\n")
		end
	      	if uci_vif.ChPlanningOBSSThresh_6G ~= nil then
	              io.write("ChPlanningOBSSThresh_6G=", uci_vif.ChPlanningOBSSThresh_6G, "\n")
		end
	      	if uci_vif.ChPlanningR2MonitorTimeoutSecs ~= nil then
	              io.write("ChPlanningR2MonitorTimeoutSecs=", uci_vif.ChPlanningR2MonitorTimeoutSecs, "\n")
		end
	      	if uci_vif.ChPlanningR2MonitorProhibitSecs ~= nil then
	              io.write("ChPlanningR2MonitorProhibitSecs=", uci_vif.ChPlanningR2MonitorProhibitSecs, "\n")
		end
	      	if uci_vif.ChPlanningR2MetricReportingInterval ~= nil then
	              io.write("ChPlanningR2MetricReportingInterval=", uci_vif.ChPlanningR2MetricReportingInterval, "\n")
		end
	      	if uci_vif.ChPlanningR2MinScoreMargin ~= nil then
	              io.write("ChPlanningR2MinScoreMargin=", uci_vif.ChPlanningR2MinScoreMargin, "\n")
		end
	      	if uci_vif.MetricRepIntv ~= nil then
	              io.write("MetricRepIntv=", uci_vif.MetricRepIntv, "\n")
		end
	      	if uci_vif.MetricPolicyRcpi_24G ~= nil then
	              io.write("MetricPolicyRcpi_24G=", uci_vif.MetricPolicyRcpi_24G, "\n")
		end
	      	if uci_vif.MetricPolicyRcpi_5GL ~= nil then
	              io.write("MetricPolicyRcpi_5GL=", uci_vif.MetricPolicyRcpi_5GL, "\n")
		end
	      	if uci_vif.MetricPolicyRcpi_5GH ~= nil then
	              io.write("MetricPolicyRcpi_5GH=", uci_vif.MetricPolicyRcpi_5GH, "\n")
		end
	      	if uci_vif.MetricPolicyRcpi_6G ~= nil then
	              io.write("MetricPolicyRcpi_6G=", uci_vif.MetricPolicyRcpi_6G, "\n")
		end
		io.close()
		os.execute("cp /tmp/mapd_strng.txt /etc/mapd_strng.conf")
		os.remove(file_name)
	end

-- converting mapd_cfg file

	local file_name = "/tmp/mapd_cfg.txt"
        local file
	for _, uci_vif in pairs(uci_cfg["mapd_cfg"]) do
	        file = io.open(file_name, "w+")
		io.output(file)
		if uci_vif.mode ~= nil then
			io.write("mode=", uci_vif.mode, "\n")
		end
		if uci_vif.lan_interface ~= nil then
			io.write("lan_interface=", uci_vif.lan_interface, "\n")
		end
		if uci_vif.wan_interface ~= nil then
			io.write("wan_interface=", uci_vif.wan_interface, "\n")
		end
		if uci_vif.DeviceRole ~= nil then
			io.write("DeviceRole=", uci_vif.DeviceRole, "\n")
		end
		if uci_vif.APSteerRssiTh ~= nil then
			io.write("APSteerRssiTh=", uci_vif.APSteerRssiTh, "\n")
		end
		if uci_vif.BhPriority2G ~= nil then
			io.write("BhPriority2G=", uci_vif.BhPriority2G, "\n")
		end
		if uci_vif.BhPriority5GL ~= nil then
			io.write("BhPriority5GL=", uci_vif.BhPriority5GL, "\n")
		end
		if uci_vif.BhPriority5GH ~= nil then
			io.write("BhPriority5GH=", uci_vif.BhPriority5GH, "\n")
		end
		if uci_vif.BhPriority6G ~= nil then
			io.write("BhPriority6G=", uci_vif.BhPriority6G, "\n")
		end
		if uci_vif.ChPlanningIdleByteCount ~= nil  and uci_vif.ChPlanningIdleByteCount ~= ' ' then
			io.write("ChPlanningIdleByteCount=", uci_vif.ChPlanningIdleByteCount, "\n")
		else
	              io.write("ChPlanningIdleByteCount=", "\n")
		end
		if uci_vif.ChPlanningIdleTime ~= nil and uci_vif.ChPlanningIdleTime ~= ' ' then
			io.write("ChPlanningIdleTime=", uci_vif.ChPlanningIdleTime, "\n")
		else
	              io.write("ChPlanningIdleTime=", "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel5G ~= nil and uci_vif.ChPlanningUserPreferredChannel5G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel5G=", uci_vif.ChPlanningUserPreferredChannel5G, "\n")
		else
	              io.write("ChPlanningUserPreferredChannel5G=", "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel5GH ~= nil and uci_vif.ChPlanningUserPreferredChannel5GH ~= ' ' then
			io.write("ChPlanningUserPreferredChannel5GH=", uci_vif.ChPlanningUserPreferredChannel5GH, "\n")
		else
	              io.write("ChPlanningUserPreferredChannel5GH=", "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel6G ~= nil and uci_vif.ChPlanningUserPreferredChannel6G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel6G=", uci_vif.ChPlanningUserPreferredChannel6G, "\n")
		else
	              io.write("ChPlanningUserPreferredChannel6G=", "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel2G ~= nil and uci_vif.ChPlanningUserPreferredChannel2G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel2G=", uci_vif.ChPlanningUserPreferredChannel2G, "\n")
		else
			io.write("ChPlanningUserPreferredChannel2G=", "\n")
		end
		if uci_vif.ChPlanningInitTimeout ~= nil then
			io.write("ChPlanningInitTimeout=", uci_vif.ChPlanningInitTimeout, "\n")
		end
		if uci_vif.NtwrkOptBootupWaitTime ~= nil then
			io.write("NtwrkOptBootupWaitTime=", uci_vif.NtwrkOptBootupWaitTime, "\n")
		end
		if uci_vif.NtwrkOptConnectWaitTime ~= nil then
			io.write("NtwrkOptConnectWaitTime=", uci_vif.NtwrkOptConnectWaitTime, "\n")
		end
		if uci_vif.NtwrkOptDisconnectWaitTime ~= nil then
			io.write("NtwrkOptDisconnectWaitTime=", uci_vif.NtwrkOptDisconnectWaitTime, "\n")
		end
		if uci_vif.NtwrkOptPeriodicity ~= nil then
			io.write("NtwrkOptPeriodicity=", uci_vif.NtwrkOptPeriodicity, "\n")
		end
		if uci_vif.NetworkOptimizationScoreMargin ~= nil then
			io.write("NetworkOptimizationScoreMargin=", uci_vif.NetworkOptimizationScoreMargin, "\n")
		end
		if uci_vif.BandSwitchTime ~= nil and uci_vif.BandSwitchTime ~= ' ' then
			io.write("BandSwitchTime=", uci_vif.BandSwitchTime, "\n")
		else
	              io.write("BandSwitchTime=", "\n")
		end
		if uci_vif.ScanThreshold2g ~= nil then
			io.write("ScanThreshold2g=", uci_vif.ScanThreshold2g, "\n")
		end
		if uci_vif.ScanThreshold5g ~= nil then
			io.write("ScanThreshold5g=", uci_vif.ScanThreshold5g, "\n")
		end
		if uci_vif.ScanThreshold6g ~= nil then
			io.write("ScanThreshold6g=", uci_vif.ScanThreshold6g, "\n")
		end
		if uci_vif.LowRSSIAPSteerEdge_RE ~= nil then
			io.write("LowRSSIAPSteerEdge_RE=", uci_vif.LowRSSIAPSteerEdge_RE, "\n")
		end
		if uci_vif.BhProfile0Valid ~= nil then
			io.write("BhProfile0Valid=", uci_vif.BhProfile0Valid, "\n")
		end
		if uci_vif.BhProfile0Ssid ~= nil then
			io.write("BhProfile0Ssid=", uci_vif.BhProfile0Ssid, "\n")
		end
		if uci_vif.BhProfile0AuthMode ~= nil then
			io.write("BhProfile0AuthMode=", uci_vif.BhProfile0AuthMode, "\n")
		end
		if uci_vif.BhProfile0EncrypType ~= nil then
			io.write("BhProfile0EncrypType=", uci_vif.BhProfile0EncrypType, "\n")
		end
		if uci_vif.BhProfile0WpaPsk ~= nil then
			io.write("BhProfile0WpaPsk=", uci_vif.BhProfile0WpaPsk, "\n")
		end
		if uci_vif.BhProfile0RaID ~= nil then
			io.write("BhProfile0RaID=", uci_vif.BhProfile0RaID, "\n")
		end
		if uci_vif.BhProfile1Ssid ~= nil then
			io.write("BhProfile1Ssid=", uci_vif.BhProfile1Ssid, "\n")
		end
		if uci_vif.BhProfile1AuthMode ~= nil then
			io.write("BhProfile1AuthMode=", uci_vif.BhProfile1AuthMode, "\n")
		end
		if uci_vif.BhProfile1EncrypType ~= nil then
			io.write("BhProfile1EncrypType=", uci_vif.BhProfile1EncrypType, "\n")
		end
		if uci_vif.BhProfile1WpaPsk ~= nil then
			io.write("BhProfile1WpaPsk=", uci_vif.BhProfile1WpaPsk, "\n")
		end
		if uci_vif.BhProfile1Valid ~= nil then
			io.write("BhProfile1Valid=", uci_vif.BhProfile1Valid, "\n")
		end
		if uci_vif.BhProfile1RaID ~= nil then
			io.write("BhProfile1RaID=", uci_vif.BhProfile1RaID, "\n")
		end
		if uci_vif.BhProfile2Ssid ~= nil then
			io.write("BhProfile2Ssid=", uci_vif.BhProfile2Ssid, "\n")
		end
		if uci_vif.BhProfile2AuthMode ~= nil then
			io.write("BhProfile2AuthMode=", uci_vif.BhProfile2AuthMode, "\n")
		end
		if uci_vif.BhProfile2EncrypType ~= nil then
			io.write("BhProfile2EncrypType=", uci_vif.BhProfile2EncrypType, "\n")
		end
		if uci_vif.BhProfile2WpaPsk ~= nil then
			io.write("BhProfile2WpaPsk=", uci_vif.BhProfile2WpaPsk, "\n")
		end
		if uci_vif.BhProfile2Valid ~= nil then
			io.write("BhProfile2Valid=", uci_vif.BhProfile2Valid, "\n")
		end
		if uci_vif.BhProfile2RaID ~= nil then
			io.write("BhProfile2RaID=", uci_vif.BhProfile2RaID, "\n")
		end
		if uci_vif.ChPlanningEnable ~= nil then
			io.write("ChPlanningEnable=", uci_vif.ChPlanningEnable, "\n")
		end
		if uci_vif.ChPlanningEnableR2 ~= nil and uci_vif.ChPlanningEnableR2 ~= ' ' then
			io.write("ChPlanningEnableR2=", uci_vif.ChPlanningEnableR2, "\n")
		else
	              io.write("ChPlanningEnableR2=", "\n")
		end
		if uci_vif.SteerEnable ~= nil then
			io.write("SteerEnable=", uci_vif.SteerEnable, "\n")
		end
		if uci_vif.NetworkOptimizationEnabled ~= nil then
			io.write("NetworkOptimizationEnabled=", uci_vif.NetworkOptimizationEnabled, "\n")
		end
		if uci_vif.AutoBHSwitching ~= nil then
			io.write("AutoBHSwitching=", uci_vif.AutoBHSwitching, "\n")
		end
		if uci_vif.DhcpCtl ~= nil then
			io.write("DhcpCtl=", uci_vif.DhcpCtl, "\n")
		end
		if uci_vif.ThirdPartyConnection ~= nil then
			io.write("ThirdPartyConnection=", uci_vif.ThirdPartyConnection, "\n")
		end
		if uci_vif.MAP_QuickChChange ~= nil then
			io.write("MAP_QuickChChange=", uci_vif.MAP_QuickChChange, "\n")
		end
		if uci_vif.bss_config_priority ~= nil then
			io.write("bss_config_priority=", uci_vif.bss_config_priority, "\n")
		end
		if uci_vif.DualBH ~= nil  and uci_vif.DualBH ~= ' ' then
			io.write("DualBH=", uci_vif.DualBH, "\n")
		else
			io.write("DualBH=", "\n")
		end
		if uci_vif.MetricRepIntv ~= nil then
			io.write("MetricRepIntv=", uci_vif.MetricRepIntv, "\n")
		end
		if uci_vif.MaxAllowedScan ~= nil and uci_vif.MaxAllowedScan ~= ' ' then
			io.write("MaxAllowedScan=", uci_vif.MaxAllowedScan, "\n")
		else
	              io.write("MaxAllowedScan=", "\n")
		end
		if uci_vif.BHSteerTimeout ~= nil then
			io.write("BHSteerTimeout=", uci_vif.BHSteerTimeout, "\n")
		end
		if uci_vif.NtwrkOptPostCACTriggerTime ~= nil then
			io.write("NtwrkOptPostCACTriggerTime=", uci_vif.NtwrkOptPostCACTriggerTime, "\n")
		end
		if uci_vif.role_detection_external ~= nil then
			io.write("role_detection_external=", uci_vif.role_detection_external, "\n")
		end
		if uci_vif.NetworkOptPrefer5Gover2G ~= nil then
			io.write("NetworkOptPrefer5Gover2G=", uci_vif.NetworkOptPrefer5Gover2G, "\n")
		end
		if uci_vif.NetworkOptPrefer5Gover2GRetryCnt ~= nil then
			io.write("NetworkOptPrefer5Gover2GRetryCnt=", uci_vif.NetworkOptPrefer5Gover2GRetryCnt, "\n")
		end
		if uci_vif.NonMAPAPEnable ~= nil then
			io.write("NonMAPAPEnable=", uci_vif.NonMAPAPEnable, "\n")
		end
		if uci_vif.CentralizedSteering ~= nil then
			io.write("CentralizedSteering=", uci_vif.CentralizedSteering, "\n")
		end
		if uci_vif.ChPlanningEnableR2withBW ~= nil and uci_vif.ChPlanningEnableR2withBW ~= ' ' then
			io.write("ChPlanningEnableR2withBW=", uci_vif.ChPlanningEnableR2withBW, "\n")
		else
	              io.write("ChPlanningEnableR2withBW=", "\n")
		end
		if uci_vif.DivergentChPlanning ~= nil then
			io.write("DivergentChPlanning=", uci_vif.DivergentChPlanning, "\n")
		end
		if uci_vif.LastMapMode ~= nil then
			io.write("LastMapMode=", uci_vif.LastMapMode, "\n")
		end
		if uci_vif.NtwrkOptDataCollectionTime ~= nil then
			io.write("NtwrkOptDataCollectionTime=", uci_vif.NtwrkOptDataCollectionTime, "\n")
		end
		if uci_vif.ChPlanningScanValidTime ~= nil then
			io.write("ChPlanningScanValidTime=", uci_vif.ChPlanningScanValidTime, "\n")
		end
		if uci_vif.NetOptUserSetPriority ~= nil then
			io.write("NetOptUserSetPriority=", uci_vif.NetOptUserSetPriority, "\n")
		end
		if uci_vif.DESerialNumber ~= nil then
			io.write("DESerialNumber=", uci_vif.DESerialNumber, "\n")
		end
		if uci_vif.DESoftwareVersion ~= nil then
			io.write("DESoftwareVersion=", uci_vif.DESoftwareVersion, "\n")
		end
		if uci_vif.DEExecutionEnv ~= nil then
			io.write("DEExecutionEnv=", uci_vif.DEExecutionEnv, "\n")
		end
		if uci_vif.DEChipsetVendor ~= nil then
			io.write("DEChipsetVendor=", uci_vif.DEChipsetVendor, "\n")
		end
		if uci_vif.DEStaConEventPath ~= nil and uci_vif.DEStaConEventPath ~= ' ' then
			io.write("DEStaConEventPath=", uci_vif.DEStaConEventPath, "\n")
		else
	              io.write("DEStaConEventPath=", "\n")
		end
		if uci_vif.SetPSCChannel_6G ~= nil then
			io.write("SetPSCChannel_6G=", uci_vif.SetPSCChannel_6G, "\n")
		end
		if uci_vif.NtwrkOptChUtilCollectionTime6G ~= nil then
			io.write("NtwrkOptChUtilCollectionTime6G=", uci_vif.NtwrkOptChUtilCollectionTime6G, "\n")
		end
		if uci_vif.NtwrkOptDataChUtilThresh6G ~= nil then
			io.write("NtwrkOptDataChUtilThresh6G=", uci_vif.NtwrkOptDataChUtilThresh6G, "\n")
		end
		if uci_vif.NtwrkOptDataChUtilEnDisable6G ~= nil then
			io.write("NtwrkOptDataChUtilEnDisable6G=", uci_vif.NtwrkOptDataChUtilEnDisable6G, "\n")
		end
		if uci_vif.NtwrkOptMldBhPriority ~= nil then
			io.write("NtwrkOptMldBhPriority=", uci_vif.NtwrkOptMldBhPriority, "\n")
		end
	      	if uci_vif.CUOverloadTh_2G ~= nil then
	              io.write("CUOverloadTh_2G=", uci_vif.CUOverloadTh_2G, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_L ~= nil then
	              io.write("CUOverloadTh_5G_L=", uci_vif.CUOverloadTh_5G_L, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_H ~= nil then
	              io.write("CUOverloadTh_5G_H=", uci_vif.CUOverloadTh_5G_H, "\n")
		end
	      	if uci_vif.CUOverloadTh_6G ~= nil then
	              io.write("CUOverloadTh_6G=", uci_vif.CUOverloadTh_6G, "\n")
		end
	      	if uci_vif.channel_setting_5gh ~= nil then
	              io.write("channel_setting_5gh=", uci_vif.channel_setting_5gh, "\n")
		end
	      	if uci_vif.channel_setting_5gl ~= nil then
	              io.write("channel_setting_5gl=", uci_vif.channel_setting_5gl, "\n")
		end
	      	if uci_vif.channel_setting_6g ~= nil then
	              io.write("channel_setting_6g=", uci_vif.channel_setting_6g, "\n")
		end
	      	if uci_vif.client_mac~= nil then
	              io.write("client_mac=", uci_vif.client_mac, "\n")
		end
	     	if uci_vif.dpp_uri  ~= nil then
	             io.write("dpp-uri=", uci_vif.dpp_uri, "\n")
		end
	      	if uci_vif.MapMode ~= nil then
	              io.write("MapMode=", uci_vif.MapMode, "\n")
		end
	      	if uci_vif.TriBand ~= nil then
	              io.write("TriBand=", uci_vif.TriBand, "\n")
		end
	      	if uci_vif.priority ~= nil then
	              io.write("priority=", uci_vif.priority, "\n")
		end
	      	if uci_vif.index ~= nil then
	              io.write("index=", uci_vif.index, "\n")
		end
	      	if uci_vif.protocol ~= nil then
	              io.write("protocol=", uci_vif.protocol, "\n")
		end
		if uci_vif.DfsSlaveEn ~= nil then
	              io.write("DfsSlaveEn=", uci_vif.DfsSlaveEn, "\n")
		end
		if uci_vif.DFSBW80Enable ~= nil then
			io.write("DFSBW80Enable=", uci_vif.DFSBW80Enable, "\n")
		end
		io.close()
		os.execute("cp /tmp/mapd_cfg.txt /etc/map/mapd_cfg")
		os.remove(file_name)
	end

--converting mapd_user file

	local file_name = "/etc/map/mapd_user.cfg"
        local file
--delete all parameter to avoid repeat
		os.execute("sed -i '/mode=/d' "..file_name)
		os.execute("sed -i '/lan_interface=/d' "..file_name)
		os.execute("sed -i '/wan_interface=/d' "..file_name)
		os.execute("sed -i '/DeviceRole=/d' "..file_name)
		os.execute("sed -i '/APSteerRssiTh=/d' "..file_name)
		os.execute("sed -i '/BhPriority2G=/d' "..file_name)
		os.execute("sed -i '/BhPriority5GL=/d' "..file_name)
		os.execute("sed -i '/BhPriority5GH=/d' "..file_name)
		os.execute("sed -i '/BhPriority6G=/d' "..file_name)
		os.execute("sed -i '/ChPlanningIdleByteCount=/d' "..file_name)
		os.execute("sed -i '/ChPlanningIdleTime=/d' "..file_name)
		os.execute("sed -i '/ChPlanningUserPreferredChannel5G=/d' "..file_name)
		os.execute("sed -i '/ChPlanningUserPreferredChannel5GH=/d' "..file_name)
		os.execute("sed -i '/ChPlanningUserPreferredChannel2G=/d' "..file_name)
		os.execute("sed -i '/ChPlanningUserPreferredChannel6G=/d' "..file_name)
		os.execute("sed -i '/ChPlanningInitTimeout=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptBootupWaitTime=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptConnectWaitTime=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptDisconnectWaitTime=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptPeriodicity=/d' "..file_name)
		os.execute("sed -i '/NetworkOptimizationScoreMargin=/d' "..file_name)
		os.execute("sed -i '/BandSwitchTime=/d' "..file_name)
		os.execute("sed -i '/ScanThreshold2g=/d' "..file_name)
		os.execute("sed -i '/ScanThreshold2g=/d' "..file_name)
		os.execute("sed -i '/ScanThreshold5g=/d' "..file_name)
		os.execute("sed -i '/ScanThreshold6g=/d' "..file_name)
		os.execute("sed -i '/LowRSSIAPSteerEdge_RE=/d' "..file_name)
		os.execute("sed -i '/ChPlanningEnable=/d' "..file_name)
		os.execute("sed -i '/ChPlanningEnableR2=/d' "..file_name)
		os.execute("sed -i '/SteerEnable=/d' "..file_name)
		os.execute("sed -i '/NetworkOptimizationEnabled=/d' "..file_name)
		os.execute("sed -i '/AutoBHSwitching=/d' "..file_name)
		os.execute("sed -i '/DhcpCtl=/d' "..file_name)
		os.execute("sed -i '/ThirdPartyConnection=/d' "..file_name)
		os.execute("sed -i '/MAP_QuickChChange=/d' "..file_name)
		os.execute("sed -i '/bss_config_priority=/d' "..file_name)
		os.execute("sed -i '/DualBH=/d' "..file_name)
		os.execute("sed -i '/MetricRepIntv=/d' "..file_name)
		os.execute("sed -i '/MaxAllowedScan=/d' "..file_name)
		os.execute("sed -i '/BHSteerTimeout=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptPostCACTriggerTime=/d' "..file_name)
		os.execute("sed -i '/role_detection_external=/d' "..file_name)
		os.execute("sed -i '/NetworkOptPrefer5Gover2G=/d' "..file_name)
		os.execute("sed -i '/NetworkOptPrefer5Gover2GRetryCnt=/d' "..file_name)
		os.execute("sed -i '/NonMAPAPEnable=/d' "..file_name)
		os.execute("sed -i '/CentralizedSteering=/d' "..file_name)
		os.execute("sed -i '/ChPlanningEnableR2withBW=/d' "..file_name)
		os.execute("sed -i '/DivergentChPlanning=/d' "..file_name)
		os.execute("sed -i '/LastMapMode=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptDataCollectionTime=/d' "..file_name)
		os.execute("sed -i '/ChPlanningScanValidTime=/d' "..file_name)
		os.execute("sed -i '/NetOptUserSetPriority=/d' "..file_name)
		os.execute("sed -i '/DESerialNumber=/d' "..file_name)
		os.execute("sed -i '/DESoftwareVersion=/d' "..file_name)
		os.execute("sed -i '/DEExecutionEnv=/d' "..file_name)
		os.execute("sed -i '/DEChipsetVendor=/d' "..file_name)
		os.execute("sed -i '/DEStaConEventPath=/d' "..file_name)
		os.execute("sed -i '/SetPSCChannel_6G=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptChUtilCollectionTime6G=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptDataChUtilThresh6G=/d' "..file_name)
		os.execute("sed -i '/NtwrkOptDataChUtilEnDisable6G=/d' "..file_name)
		os.execute("sed -i '/CUOverloadTh_2G=/d' "..file_name)
		os.execute("sed -i '/CUOverloadTh_5G_L=/d' "..file_name)
		os.execute("sed -i '/CUOverloadTh_5G_H=/d' "..file_name)
		os.execute("sed -i '/CUOverloadTh_6G=/d' "..file_name)
		os.execute("sed -i '/channel_setting_5gh=/d' "..file_name)
		os.execute("sed -i '/channel_setting_5gl=/d' "..file_name)
		os.execute("sed -i '/channel_setting_6g=/d' "..file_name)
		os.execute("sed -i '/client_mac=/d' "..file_name)
		os.execute("sed -i '/dpp-uri=/d' "..file_name)
		os.execute("sed -i '/MapMode=/d' "..file_name)
		os.execute("sed -i '/TriBand=/d' "..file_name)
		os.execute("sed -i '/priority=/d' "..file_name)
		os.execute("sed -i '/index=/d' "..file_name)
		os.execute("sed -i '/protocol=/d' "..file_name)
		os.execute("sed -i '/DFSBW80Enable=/d' "..file_name)
--
	for _, uci_vif in pairs(uci_cfg["mapd_user"]) do
	        file = io.open(file_name, "a+")
		io.output(file)
		if uci_vif.mode ~= nil then
			io.write("mode=", uci_vif.mode, "\n")
		end
		if uci_vif.lan_interface ~= nil then
			io.write("lan_interface=", uci_vif.lan_interface, "\n")
		end
		if uci_vif.wan_interface ~= nil then
			io.write("wan_interface=", uci_vif.wan_interface, "\n")
		end
		if uci_vif.DeviceRole ~= nil then
			io.write("DeviceRole=", uci_vif.DeviceRole, "\n")
		end
		if uci_vif.APSteerRssiTh ~= nil then
			io.write("APSteerRssiTh=", uci_vif.APSteerRssiTh, "\n")
		end
		if uci_vif.BhPriority2G ~= nil then
			io.write("BhPriority2G=", uci_vif.BhPriority2G, "\n")
		end
		if uci_vif.BhPriority5GL ~= nil then
			io.write("BhPriority5GL=", uci_vif.BhPriority5GL, "\n")
		end
		if uci_vif.BhPriority5GH ~= nil then
			io.write("BhPriority5GH=", uci_vif.BhPriority5GH, "\n")
		end
		if uci_vif.BhPriority6G ~= nil then
			io.write("BhPriority6G=", uci_vif.BhPriority6G, "\n")
		end
		if uci_vif.ChPlanningIdleByteCount ~= nil and uci_vif.ChPlanningIdleByteCount ~= ' ' then
			io.write("ChPlanningIdleByteCount=", uci_vif.ChPlanningIdleByteCount, "\n")
		end
		if uci_vif.ChPlanningIdleTime ~= nil and uci_vif.ChPlanningIdleTime ~= ' ' then
			io.write("ChPlanningIdleTime=", uci_vif.ChPlanningIdleTime, "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel5G ~= nil and uci_vif.ChPlanningUserPreferredChannel5G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel5G=", uci_vif.ChPlanningUserPreferredChannel5G, "\n")
		end
		if uci_vif.ChPlanningEnableR2 ~= nil and uci_vif.ChPlanningEnableR2 ~= ' ' then
			io.write("ChPlanningEnableR2=", uci_vif.ChPlanningEnableR2, "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel5GH ~= nil and uci_vif.ChPlanningUserPreferredChannel5GH ~= ' ' then
			io.write("ChPlanningUserPreferredChannel5GH=", uci_vif.ChPlanningUserPreferredChannel5GH, "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel6G ~= nil and uci_vif.ChPlanningUserPreferredChannel6G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel6G=", uci_vif.ChPlanningUserPreferredChannel6G, "\n")
		end
		if uci_vif.ChPlanningUserPreferredChannel2G ~= nil and uci_vif.ChPlanningUserPreferredChannel2G ~= ' ' then
			io.write("ChPlanningUserPreferredChannel2G=", uci_vif.ChPlanningUserPreferredChannel2G, "\n")
		end
		if uci_vif.ChPlanningInitTimeout ~= nil then
			io.write("ChPlanningInitTimeout=", uci_vif.ChPlanningInitTimeout, "\n")
		end
		if uci_vif.NtwrkOptBootupWaitTime ~= nil then
			io.write("NtwrkOptBootupWaitTime=", uci_vif.NtwrkOptBootupWaitTime, "\n")
		end
		if uci_vif.NtwrkOptConnectWaitTime ~= nil then
			io.write("NtwrkOptConnectWaitTime=", uci_vif.NtwrkOptConnectWaitTime, "\n")
		end
		if uci_vif.NtwrkOptDisconnectWaitTime ~= nil then
			io.write("NtwrkOptDisconnectWaitTime=", uci_vif.NtwrkOptDisconnectWaitTime, "\n")
		end
		if uci_vif.NtwrkOptPeriodicity ~= nil then
			io.write("NtwrkOptPeriodicity=", uci_vif.NtwrkOptPeriodicity, "\n")
		end
		if uci_vif.NetworkOptimizationScoreMargin ~= nil then
			io.write("NetworkOptimizationScoreMargin=", uci_vif.NetworkOptimizationScoreMargin, "\n")
		end
		if uci_vif.BandSwitchTime ~= nil or uci_vif.BandSwitchTime == '' then
			io.write("BandSwitchTime=", uci_vif.BandSwitchTime, "\n")
		end
		if uci_vif.ScanThreshold2g ~= nil then
			io.write("ScanThreshold2g=", uci_vif.ScanThreshold2g, "\n")
		end
		if uci_vif.ScanThreshold5g ~= nil then
			io.write("ScanThreshold5g=", uci_vif.ScanThreshold5g, "\n")
		end
		if uci_vif.ScanThreshold6g ~= nil then
			io.write("ScanThreshold6g=", uci_vif.ScanThreshold6g, "\n")
		end
		if uci_vif.LowRSSIAPSteerEdge_RE ~= nil then
			io.write("LowRSSIAPSteerEdge_RE=", uci_vif.LowRSSIAPSteerEdge_RE, "\n")
		end
		if uci_vif.ChPlanningEnable ~= nil then
			io.write("ChPlanningEnable=", uci_vif.ChPlanningEnable, "\n")
		end
		if uci_vif.SteerEnable ~= nil then
			io.write("SteerEnable=", uci_vif.SteerEnable, "\n")
		end
		if uci_vif.NetworkOptimizationEnabled ~= nil then
			io.write("NetworkOptimizationEnabled=", uci_vif.NetworkOptimizationEnabled, "\n")
		end
		if uci_vif.AutoBHSwitching ~= nil then
			io.write("AutoBHSwitching=", uci_vif.AutoBHSwitching, "\n")
		end
		if uci_vif.DhcpCtl ~= nil then
			io.write("DhcpCtl=", uci_vif.DhcpCtl, "\n")
		end
		if uci_vif.ThirdPartyConnection ~= nil then
			io.write("ThirdPartyConnection=", uci_vif.ThirdPartyConnection, "\n")
		end
		if uci_vif.MAP_QuickChChange ~= nil then
			io.write("MAP_QuickChChange=", uci_vif.MAP_QuickChChange, "\n")
		end
		if uci_vif.bss_config_priority ~= nil then
			io.write("bss_config_priority=", uci_vif.bss_config_priority, "\n")
		end
		if uci_vif.DualBH ~= nil and uci_vif.DualBH ~= ' ' then
			io.write("DualBH=", uci_vif.DualBH, "\n")
		end
		if uci_vif.MetricRepIntv ~= nil then
			io.write("MetricRepIntv=", uci_vif.MetricRepIntv, "\n")
		end
		if uci_vif.MaxAllowedScan ~= nil and uci_vif.MaxAllowedScan ~= ' ' then
			io.write("MaxAllowedScan=", uci_vif.MaxAllowedScan, "\n")
		end
		if uci_vif.BHSteerTimeout ~= nil then
			io.write("BHSteerTimeout=", uci_vif.BHSteerTimeout, "\n")
		end
		if uci_vif.NtwrkOptPostCACTriggerTime ~= nil then
			io.write("NtwrkOptPostCACTriggerTime=", uci_vif.NtwrkOptPostCACTriggerTime, "\n")
		end
		if uci_vif.role_detection_external ~= nil then
			io.write("role_detection_external=", uci_vif.role_detection_external, "\n")
		end
		if uci_vif.NetworkOptPrefer5Gover2G ~= nil then
			io.write("NetworkOptPrefer5Gover2G=", uci_vif.NetworkOptPrefer5Gover2G, "\n")
		end
		if uci_vif.NetworkOptPrefer5Gover2GRetryCnt ~= nil then
			io.write("NetworkOptPrefer5Gover2GRetryCnt=", uci_vif.NetworkOptPrefer5Gover2GRetryCnt, "\n")
		end
		if uci_vif.NonMAPAPEnable ~= nil then
			io.write("NonMAPAPEnable=", uci_vif.NonMAPAPEnable, "\n")
		end
		if uci_vif.CentralizedSteering ~= nil then
			io.write("CentralizedSteering=", uci_vif.CentralizedSteering, "\n")
		end
		if uci_vif.ChPlanningEnableR2withBW ~= nil and uci_vif.ChPlanningEnableR2withBW ~= ' ' then
			io.write("ChPlanningEnableR2withBW=", uci_vif.ChPlanningEnableR2withBW, "\n")
		end
		if uci_vif.DivergentChPlanning ~= nil then
			io.write("DivergentChPlanning=", uci_vif.DivergentChPlanning, "\n")
		end
		if uci_vif.LastMapMode ~= nil then
			io.write("LastMapMode=", uci_vif.LastMapMode, "\n")
		end
		if uci_vif.NtwrkOptDataCollectionTime ~= nil then
			io.write("NtwrkOptDataCollectionTime=", uci_vif.NtwrkOptDataCollectionTime, "\n")
		end
		if uci_vif.ChPlanningScanValidTime ~= nil then
			io.write("ChPlanningScanValidTime=", uci_vif.ChPlanningScanValidTime, "\n")
		end
		if uci_vif.NetOptUserSetPriority ~= nil then
			io.write("NetOptUserSetPriority=", uci_vif.NetOptUserSetPriority, "\n")
		end
		if uci_vif.DESerialNumber ~= nil then
			io.write("DESerialNumber=", uci_vif.DESerialNumber, "\n")
		end
		if uci_vif.DESoftwareVersion ~= nil then
			io.write("DESoftwareVersion=", uci_vif.DESoftwareVersion, "\n")
		end
		if uci_vif.DEExecutionEnv ~= nil then
			io.write("DEExecutionEnv=", uci_vif.DEExecutionEnv, "\n")
		end
		if uci_vif.DEChipsetVendor ~= nil then
			io.write("DEChipsetVendor=", uci_vif.DEChipsetVendor, "\n")
		end
		if uci_vif.DEStaConEventPath ~= nil and uci_vif.DEStaConEventPath ~= ' ' then
			io.write("DEStaConEventPath=", uci_vif.DEStaConEventPath, "\n")
		end
		if uci_vif.SetPSCChannel_6G ~= nil then
			io.write("SetPSCChannel_6G=", uci_vif.SetPSCChannel_6G, "\n")
		end
		if uci_vif.NtwrkOptChUtilCollectionTime6G ~= nil then
			io.write("NtwrkOptChUtilCollectionTime6G=", uci_vif.NtwrkOptChUtilCollectionTime6G, "\n")
		end
		if uci_vif.NtwrkOptDataChUtilThresh6G ~= nil then
			io.write("NtwrkOptDataChUtilThresh6G=", uci_vif.NtwrkOptDataChUtilThresh6G, "\n")
		end
		if uci_vif.NtwrkOptDataChUtilEnDisable6G ~= nil then
			io.write("NtwrkOptDataChUtilEnDisable6G=", uci_vif.NtwrkOptDataChUtilEnDisable6G, "\n")
		end
	      	if uci_vif.CUOverloadTh_2G ~= nil then
	              io.write("CUOverloadTh_2G=", uci_vif.CUOverloadTh_2G, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_L ~= nil then
	              io.write("CUOverloadTh_5G_L=", uci_vif.CUOverloadTh_5G_L, "\n")
		end
	      	if uci_vif.CUOverloadTh_5G_H ~= nil then
	              io.write("CUOverloadTh_5G_H=", uci_vif.CUOverloadTh_5G_H, "\n")
		end
	      	if uci_vif.CUOverloadTh_6G ~= nil then
	              io.write("CUOverloadTh_6G=", uci_vif.CUOverloadTh_6G, "\n")
		end
	      	if uci_vif.channel_setting_5gh ~= nil then
	              io.write("channel_setting_5gh=", uci_vif.channel_setting_5gh, "\n")
		end
	      	if uci_vif.channel_setting_5gl ~= nil then
	              io.write("channel_setting_5gl=", uci_vif.channel_setting_5gl, "\n")
		end
	      	if uci_vif.channel_setting_6g ~= nil then
	              io.write("channel_setting_6g=", uci_vif.channel_setting_6g, "\n")
		end
	      	if uci_vif.client_mac~= nil then
	              io.write("client_mac=", uci_vif.client_mac, "\n")
		end
	     	if uci_vif.dpp_uri  ~= nil then
	             io.write("dpp-uri=", uci_vif.dpp_uri, "\n")
		end
	      	if uci_vif.MapMode ~= nil then
	              io.write("MapMode=", uci_vif.MapMode, "\n")
		end
	      	if uci_vif.TriBand ~= nil then
	              io.write("TriBand=", uci_vif.TriBand, "\n")
		end
	      	if uci_vif.priority ~= nil then
	              io.write("priority=", uci_vif.priority, "\n")
		end
	      	if uci_vif.index ~= nil then
	              io.write("index=", uci_vif.index, "\n")
		end
	      	if uci_vif.protocol ~= nil then
	              io.write("protocol=", uci_vif.protocol, "\n")
		end
		if uci_vif.DFSBW80Enable ~= nil then
			io.write("DFSBW80Enable=", uci_vif.DFSBW80Enable, "\n")
		end
		io.close()
	end

--converting wts file

	local file_name = "/tmp/wts_bss_info_config"
	local fd = io.open(file_name, "w")
	local file
	local index = 1
	local ssid
	local pwd
	fd:write('#ucc_bss_info\n')
    for _, uci_vif in pairs(uci_cfg["iface"]) do
        ssid = uci2map.__trim(uci2map.read_pipe("uci get mapd." ..index..".ssid"))
        local tmpSsid = ssid:gsub("\\", "\\\\")
        local tmpSsid = tmpSsid:gsub("%s", "\\ ")
        pwd = uci2map.__trim(uci2map.read_pipe("uci get mapd." ..index..".PSK"))
        fd:write(
            index..','..uci_vif.mac..' '..
            uci_vif.radio ..' '..
            tmpSsid..' '..
            uci_vif.authmode..' '..
            uci_vif.EncryptType..' '..
            pwd..' '..
            uci_vif.bhbss..' '..
            uci_vif.fhbss..' hidden-' ..
            uci_vif.hidden ..' '..
            uci_vif.vlan..' '..
            uci_vif.pvid..' '..
            uci_vif.pcp..' mld_groupID '..
	    uci_vif.mld_groupID ..'\n')
        index = index + 1
    end
	io.close(fd)
	os.execute("cp /tmp/wts_bss_info_config /etc/map/wts_bss_info_config")
	os.remove(file_name)
end

function uci_apply_1905d_cfg()
        local file_name = "/etc/map/1905d.cfg"
        local file
	local uciCfgfile = "/etc/config/1905d_cfg"
        local uci_cfg
        uci_cfg = uci_load(uciCfgfile)
        for _, uci_vif in pairs(uci_cfg["iface"]) do
                os.execute("sed -i '/map_ver=/d' "..file_name)
                file = io.open(file_name, "a+")
                io.output(file)
                if uci_vif.map_ver ~= nil then
                      io.write("map_ver=", uci_vif.map_ver, "\n")
                end
                io.close()
        end
end

uci_apply_mapd_configuration()
uci_apply_1905d_cfg()
