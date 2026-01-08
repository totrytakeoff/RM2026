按你当前工程（RTT + OpenOCD + SWD），直接照抄这一套跑：

**终端 A：启动 OpenOCD**

```bash
cd /home/myself/workspace/RM2026 && openocd -f config/openocd/openocd_dap.cfg 
```

**终端 B：telnet 进 OpenOCD，开启 RTT server**

```bash
telnet localhost 4444 
```

进去后依次输入这三行（每行回车）：

`rtt setup 0x20000000 0x20000 "SEGGER RTT" `

`rtt start `

`rtt server start 19021 0 `




rtt setup 0x20000000 0x20000 "SEGGER RTT"

rtt start

rtt server start 19021 0

**终端 C：连接 RTT 输出（看 LOGINFO/LOGERROR）**

`nc localhost 19021`

nc localhost 19021

如果 **telnet localhost 4444** 里提示 **rtt** 是 unknown command，把那行报错原样发我（说明你这版 OpenOCD 没带 RTT 功能，需要换 OpenOCD 或改成串口/SWO）。
