应用协议 V0： OPC （目前只实现了OPC协议）


发送流程：
L3:   stp_sendmsg()/stp_sendmsgs()
            -->
			
L2:   opc_output()/opc_output_bufv()
            -->
			
L1:   stp_output()
	        -->
			
L1:   stp_send() (stp-thread)
			-->
			
	  driver send
            
			
接收流程：

L3:	  user callback
		 <--
L2:	  opc_input()
	  
		 <--
L1:	  stp_l2_input() (stp-thread)
	  
         <--
L1:	  stp_input()
	  
	     <--
	  driver receive