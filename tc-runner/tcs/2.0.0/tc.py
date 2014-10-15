#
#
TcRunner().run_remote(host = 'hostA', iface = 'eth0', cmd = 'ls -l', log = '1.log')
TcRunner().run_remote(host = 'hostA', iface = 'eth0', cmd = 'ls -l', log = '2.log')
TcRunner().run_remote(host = 'hostA', iface = 'eth0', cmd = 'ping 127.0.0.1', log = '3.log')
TcRunner().run_remote(host = 'hostA', iface = 'eth0', cmd = 'xmessage \"hello\"', log = '4.log')
TcRunner().wait(2)
TcRunner().assert_log(text = '127.0.0.1', log = '3.log')
TcRunner().stop_remote_processes()




