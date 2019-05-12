#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import subprocess
import fhqtest
import signal
import time
import traceback

fhqtest.print_header(" > > > TESTS: begin ")

fhqtest.print_bold("Start fhq-server... ")
wd = os.getcwd()
print(wd)
os.chdir(wd + "/../fhq-server")
p_fhq_server = subprocess.Popen([wd + '/../fhq-server/fhq-server', '-wd', '../data_travis', 'start'])
os.chdir(wd)

wait_max = 20
wait_i = 0
result_check_port = False
while wait_i < wait_max:
    wait_i = wait_i + 1
    time.sleep(1)
    result_check_port = fhqtest.check_port('127.0.0.1', 1234)
    if not result_check_port:
        print(" =====> " + str(wait_i) + ": port not available... ")
    else:
        break

if not result_check_port:
    print("Port not available... failed")
    exit(-1)
else:
    print("Port available... OK!")

try:
    def run_test(file_name):
        p_test = subprocess.Popen(['python', file_name])
        p_test.wait()
        if p_test.returncode != 0:
            sys.exit(-1)

    run_test('test_web_server.py')
    run_test('test_classbook.py')
    run_test('test_users.py')
    run_test('test_games.py')
    run_test('test_leaks.py')
    run_test('test_quests.py')
    run_test('test_public_events.py')
    run_test('test_scoreboard.py')

    # last step
    run_test('test_stats.py')
    
except Exception as e:
    fhqtest.log_err(str(e))
    traceback.print_exc(file=sys.stdout)
    fhqtest.log_err("Some tests wrong")
    exit(-1)
finally:
    fhqtest.print_header(" > > > TESTS: end ")
    print("Kill process " + str(p_fhq_server.pid))
    fhqtest.print_success("All tests passed")
    os.kill(p_fhq_server.pid, signal.SIGKILL)
