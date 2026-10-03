# xAT hello_uart scripted GDB session (TASK-001 AC6).
# Run with: python3 scripts/operations.py debug --batch src/hello_uart/gdb/smoke_test.gdb
# Before this script runs, operations.py loads the ELF symbols, connects with
# "target extended-remote <host>:<port>" and resets/halts the MCU with the reset command
# of the selected debug server (pyOCD: "monitor reset halt", LinkServer: "load" + "monitor reset",
# J-Link: "monitor reset"),
# so the script itself is backend independent. Any failing command aborts the batch run
# with a non-zero exit code.
set pagination off
set confirm off

echo \n== continue to main ==\n
break main
continue
info registers pc

echo \n== read g_tickCount at main ==\n
print g_tickCount

echo \n== step over board init and boot message ==\n
next
next
info line *$pc

echo \n== resume until one tick has been printed ==\n
delete
break SDK_DelayAtLeastUs
continue
continue
print g_tickCount
if g_tickCount < 1
  echo FAIL: g_tickCount did not increase\n
  quit 1
end

echo \n== detach (target keeps running) ==\n
delete
detach
echo \nSMOKE_TEST_DONE\n
