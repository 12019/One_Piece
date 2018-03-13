void wms_bc_mm_event_notify(
  wms_bc_mm_event_e_type         event,
  wms_bc_mm_event_info_s_type   *bc_mm_event_info_ptr
)

wms_bc_mm_process_cmd(wms_cmd_type  *cmd_ptr){
...
switch(cmd_ptr->hdr.cmd_id )
...
case WMS_CMD_BC_MM_ADD_SRV:

wms_bc_mm_add_srv_proc(){

[0018/0002] MSG 05:07:42.629 Wireless Messaging Services/High [             wmsbc.c   4593] WMS_CMD_BC_MM_ADD_SRV: msg mode = 1  

wms_bc_gw_cb_enable_check()
[0018/0002] MSG 05:07:42.665 Wireless Messaging Services/High [             wmsbc.c   2571] GW CB CONF=1, PREF = 2, num_sel = 11  

[0018/0002] MSG 05:07:42.665 Wireless Messaging Services/High [             wmsbc.c   4671] GW CB enable=1  

wms_bc_gw_cb_enable_to_nas()
[0018/0002] MSG 05:07:42.665 Wireless Messaging Services/High [             wmsbc.c   2758] Sending to NAS: GW CB enable=1, as_id=0  
[0018/0003] MSG 05:07:42.665 Wireless Messaging Services/Error [             wmsbc.c   2772] Do not send search list to NAS/ LTE RRC while in LPM  

}
}

wms_bc_mm_event_notify()
[0018/0002] MSG 05:07:42.665 Wireless Messaging Services/High [             wmsbc.c   1543] Notify: 6 WMS_BC_MM_EVENT_ADD_SRVS  

=========================
===change cb_user_pref===
=========================
根因：驻网之后，上层发送QMI消息将bc pref置为1

05:07:42.799	[0x1390]	QMI Link 2 RX PDU
IFType = 1
QmiLength = 17
QmiCtlFlags = 0
QmiType = WMS
Service_Wms {
   ClientId = 1
   SduCtlFlags = REQ
   TxId = 10
   MsgType = QMI_WMS_SET_BROADCAST_ACTIVATION_MSG
   MsgLength = 5
   Service_Wms_V1 {
      Tlvs[0] {
         Type = 1
         Length = 2
         BroadcastActivationInformation Tlv {
            mode = GW
            bc_activate = Activate broadcast
         }
      }
   }
}
//QMI_WMS_SET_BROADCAST_ACTIVATION_MSG 消息的结构体
typedef struct {

  /* Mandatory */
  /*  Broadcast Activation Information */
  wms_broadcast_activation_info_type_v01 broadcast_activation_info;
  
  /*message_mode: 0 - CDMA, 1 - GW*/
  /*bc_active: 0 - disable, 1- enable*/
  
  /* Optional */
  /*  Broadcast Filtering Information */
  uint8_t activate_all_valid;  /**< Must be set to true if activate_all is being passed */
  uint8_t activate_all;
  /**<   Indicates whether to accept 3GPP2 broadcast SMS messages for all service
       categories or to accept 3GPP cell broadcast SMS messages without
       additional language preference filtering. Values: \n
       - 0x00 -- Filter 3GPP2 broadcast messages based on service categories
                 and 3GPP cell broadcast messages based on language preferences \n
       - 0x01 -- Ignore service categories or language preferences
  */
}wms_set_broadcast_activation_req_msg_v01;
			
//下发的bc pref设置			
[5000/0001] MSG 05:07:42.799 Data Services/Medium [  ds_qmi_fw_common.c    235] Dispatch the transaction(a) with (1 cmds) ctl= 0  
[5000/0001] MSG 05:07:42.799 Data Services/Medium [  ds_qmi_fw_common.c    970] Handling (qmi_svc_hdlr_ftype) qmi_wmsi_set_broadcast_activation  
			

//这里把activate_all值强制设置成了false
static dsm_item_type* qmi_wmsi_set_broadcast_activation(){
  boolean            activate_all = FALSE
  
...
  errval = (qmi_error_e_type)qmi_wmsi_from_wms_status
           (wms_bc_ms_set_pref(qmi_wmsi_global.wms_cid,
                               as_id,
                               (wms_cmd_cb_type) qmi_wms_cmd_status_cb,
                               (void *) cmd_buf_p,
                               (wms_message_mode_e_type)set_bc_activation_req_pdu->broadcast_activation_info.message_mode,
                               qmi_wmsi_to_wms_bc_pref((qmi_wmsi_bc_pref_e_type)set_bc_activation_req_pdu->broadcast_activation_info.bc_activate, activate_all)
							  )
		    );
...														
}

//set bc pref
wms_bc_pref_e_type qmi_wmsi_to_wms_bc_pref(
  qmi_wmsi_bc_pref_e_type qmi_bc_pref,
  boolean activate_all
)
{

  wms_bc_pref_e_type wms_bc_pref;

  switch (qmi_bc_pref)
  {
    case WMSI_BC_PREF_DISABLE:
      wms_bc_pref = WMS_BC_PREF_DEACTIVATE;
      break;

    case WMSI_BC_PREF_ENABLE:
      if (activate_all)
      {
        wms_bc_pref = WMS_BC_PREF_ACTIVATE_ALL;
      }
      else
      {
        wms_bc_pref = WMS_BC_PREF_ACTIVATE_TABLE;
      }
      break;

    default:
      wms_bc_pref = WMS_BC_PREF_MAX;
      break;
  }

  return wms_bc_pref;
}

//设定bc pref参数，并将CMD put给WMS
wms_status_e_type wms_bc_ms_set_pref(
  wms_client_id_type              client_id,
  sys_modem_as_id_e_type          as_id,
  wms_cmd_cb_type                 cmd_cb,
  const void                     *user_data,
  wms_message_mode_e_type         message_mode,
  wms_bc_pref_e_type              pref
){
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [               wms.c   5674] In wms_bc_ms_set_pref(client 18, as_id 0, message_mode 1)  
...
    wms_cmd_type        *cmd_ptr;

    cmd_ptr = wms_get_cmd_buf();

    if (NULL != cmd_ptr)
    {
      cmd_ptr->hdr.cmd_id      = cmd_id;
      cmd_ptr->hdr.client_id   = client_id;
      cmd_ptr->hdr.as_id       = as_id;
      cmd_ptr->hdr.appInfo_ptr = wms_get_appinfo_by_asid_and_message_mode(as_id, message_mode);
      cmd_ptr->hdr.cmd_cb      = cmd_cb;
      cmd_ptr->hdr.user_data   = (void*)user_data;
      cmd_ptr->cmd.bc_mm_set_pref.message_mode = message_mode;
      cmd_ptr->cmd.bc_mm_set_pref.bc_pref      = pref;

      wms_put_cmd( cmd_ptr );
    }
...
//put CMD to WMS
wms_put_cmd(wms_cmd_type  *cmd_ptr)
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [               wms.c   1865] Putting 402 WMS_CMD_BC_MM_SET_PREF (client 18, as_id 0, appInfo_ptr 0xc203c9b0)  

}

===================
===================
//main task
wms_task(){
...
"WMS: READY"
...
loop(){wms_process_signals();...}
}

//处理SMS相关命令和定时器的信号量
void wms_process_signals(rex_sigs_type    sigs){
...
  if( sigs & WMS_CMD_Q_SIG )
  {
    MSG_HIGH_0("got WMS_CMD_Q_SIG");

    (void) rex_clr_sigs( wms_myself, WMS_CMD_Q_SIG );

    while( ( cmd_ptr = (wms_cmd_type *) q_get( & wms_cmd_q )) != NULL )
    {
      wms_sigs = rex_get_sigs(wms_myself);

      if (wms_sigs & WMS_RPT_TIMER_SIG)
      {
        /* Also clears WMS_RPT_TIMER_SIG */
        wms_kick_dog();
      }

      wms_process_cmd( cmd_ptr );     /* actually handle it */
    }

  } /* if WMS_CMD_Q_SIG */
...
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [               wms.c   2291] got WMS_CMD_Q_SIG  

}



void wms_process_cmd(  wms_cmd_type  *cmd_ptr){
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [               wms.c   1935] Processing 402 WMS_CMD_BC_MM_SET_PREF (client 18, as_id 0, appInfo_ptr 0xc203c9b0)  

/*BC_MM commands */
  WMS_CMD_BC_MM_GET_CONFIG   = 400,        /**< Get the configuration. */
  WMS_CMD_BC_MM_GET_PREF,                  /**< Get the preference. */
  WMS_CMD_BC_MM_SET_PREF,                  /**< Set the preference. */


switch (cmd_ptr->hdr.cmd_id)
	  ...
      case WMS_CMD_BC_MM_GET_CONFIG:
      case WMS_CMD_BC_MM_GET_PREF:
      case WMS_CMD_BC_MM_SET_PREF:    //this one
      case WMS_CMD_BC_MM_GET_TABLE:
      case WMS_CMD_BC_MM_SELECT_SRV:
      case WMS_CMD_BC_MM_GET_ALL_SRV_IDS:
      case WMS_CMD_BC_MM_GET_SRV_INFO:
      case WMS_CMD_BC_MM_ADD_SRV:
      case WMS_CMD_BC_MM_DELETE_SRV:
      case WMS_CMD_BC_MM_CHANGE_SRV_INFO:
      case WMS_CMD_BC_MM_DELETE_ALL_SERVICES:
      case WMS_CMD_BC_MM_SELECT_ALL_SERVICES:
      case WMS_CMD_BC_MM_SET_PRIORITY_ALL_SERVICES:
      case WMS_CMD_BC_MM_MSG_DELETE_INDICATION:
      case WMS_CMD_BC_MM_MSG_DELETE_ALL_INDICATION:
      case WMS_CMD_BC_MM_GET_READING_PREF:
      case WMS_CMD_BC_MM_SET_READING_PREF:
#ifdef FEATURE_CDSMS_BROADCAST
      case WMS_CMD_BC_SCPT_DATA: /* Internal command */
#endif /* FEATURE_CDSMS_BROADCAST */
        wms_bc_mm_process_cmd( cmd_ptr );

wms_bc_mm_process_cmd(wms_cmd_type  *cmd_ptr){
...
switch(cmd_ptr->hdr.cmd_id )
...
case WMS_CMD_BC_MM_SET_PREF:

wms_bc_mm_set_pref_proc()
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [             wmsbc.c   3386] WMS_CMD_BC_MM_SET_PREF  

        nvi.sms_gw_cb_user_pref = (byte)cmd_ptr->cmd.bc_mm_set_pref.bc_pref;
        write_status = wms_nv_write_wait_per_subs(as_id, NV_SMS_GW_CB_USER_PREF_I, &nvi);
		
wms_nv_write_wait_per_subs()
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [             wmsnv.c    413] wms_nv_write_wait_per_subs, item=1017  

}
}

wms_bc_mm_event_notify()
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [             wmsbc.c   1543] Notify: 1 WMS_BC_MM_EVENT_PREF  

wms_bc_gw_cb_enable_check()
[0018/0002] MSG 05:07:42.799 Wireless Messaging Services/High [             wmsbc.c   2571] GW CB CONF=1, PREF = 1, num_sel = 11  

wms_bc_gw_cb_enable_to_nas()
[0018/0002] MSG 05:08:05.348 Wireless Messaging Services/High [             wmsbc.c   2758] Sending to NAS: GW CB enable=1, as_id=0  
