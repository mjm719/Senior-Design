# invoke SourceDir generated makefile for uartecho.pem3
uartecho.pem3: .libraries,uartecho.pem3
.libraries,uartecho.pem3: package/cfg/uartecho_pem3.xdl
	$(MAKE) -f C:\Users\ap58677\workspace_v7\uartecho_CC2650_LAUNCHXL_TI_CC2650F128/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\ap58677\workspace_v7\uartecho_CC2650_LAUNCHXL_TI_CC2650F128/src/makefile.libs clean
