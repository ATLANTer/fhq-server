# -*- coding: utf-8 -*-
import pytest
import json
# import asyncio
import functools
import websocket
import time
import Queue
import select
from threading import Thread

class FHQCliRecvCallback():
    
    def __init__(self):
        print("FHQCliRecvCallback created")

    def msgReceived(self, msg):
        print(msg)
        return msg;

    def wait(self):
        print("Wait")


class FHQCliRecvThread(Thread):
    __ws = None;
    __sendCommandQueue = Queue.Queue();
    __callbacks = {};
    __cmd_results = {};
    do_run = True;
    __url = None;
    __messageId = 0;

    def __init__(self, url):
        Thread.__init__(self)
        self.__url = url;

    def run(self):
        print("[FHQCliThread] Connecting to " + self.__url)
        self.__ws = websocket.create_connection(self.__url);

        # self.__ws.setblocking(0)

        print("[FHQCliThread] Connected");
        while(self.do_run):
            while not self.__sendCommandQueue.empty():
                requestJson = self.__sendCommandQueue.get();
                requestText = json.dumps(requestJson);
                # print("requestText: " + requestText);
                self.__ws.send(requestText)
            
            ready = select.select([self.__ws], [], [], 0.1)
            if ready[0]:
                responseText =  self.__ws.recv()
                responseJson = json.loads(responseText)
                cmd_m = responseJson["m"];
                self.__cmd_results[cmd_m] = json.loads(responseText);
                # print(responseText)

            # responseText = self.__ws.recv();
            # print(111)
            # amount = random.randint(3, 15)
            time.sleep(0.5)
        print("[FHQCliThread] Stopping...");
        self.__ws.close();
        print("[FHQCliThread] Stopped");

    def addToSendQueue(self, requestJson):
        self.__sendCommandQueue.put(requestJson);
        return requestJson["m"];

    def addCallback(self, msgId, callback):
        self.__callbacks[msgId] = FHQCliRecvCallback();
        print(1);

    def generateBaseCommand(self, cmd):
        self.__messageId = self.__messageId + 1;
        requestJson = {};
        msgId = "m" + str(self.__messageId);
        requestJson["m"] = msgId;
        requestJson["cmd"] = cmd;
        return requestJson;

    def hasCmdResult(self, cmd_m):
        return cmd_m in self.__cmd_results;

    def getCmdResult(self, cmd_m):
        return self.__cmd_results[cmd_m]

    def removeCmdResult(self, cmd_m):
        self.__cmd_results.pop(cmd_m, None)

class FHQCli:
    # __ws = None;
    __url = "ws://localhost:1234/";
    
    __recvThread = None
    def __init__(self, url):
        self.__url = url;
        self.connect();

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.__recvThread.do_run = False;
    
    def close(self):
        self.__recvThread.do_run = False;

    def setUrl(self, url):
        self.__url = url;

    def getUrl(self):
        return self.__url;

    def connect(self):
        self.__recvThread = FHQCliRecvThread(self.__url)
        self.__recvThread.start()
        # self.__recvThread.join()
        # print("FHQCli: Thread started");
        return True;

    def __looper(self, cmd_m):
        # TODO max loop time
        max_time = 5*10; # 5 seconds
        counter_time = 0;
        cmd_result = {};
        while True:
            counter_time = counter_time + 1;
            if self.__recvThread.hasCmdResult(cmd_m):
                cmd_result = self.__recvThread.getCmdResult(cmd_m);
                self.__recvThread.removeCmdResult(cmd_m);
                break;
            if counter_time > max_time:
                cmd_result = None
                break;
            time.sleep(0.2)
        return cmd_result;

    def login(self, email, password):
        requestJson = self.__recvThread.generateBaseCommand("login")
        requestJson["email"] = email;
        requestJson["password"] = password;
        cmd_m = self.__recvThread.addToSendQueue(requestJson);
        return self.__looper(cmd_m);

    def admin_user_create(self, email, nick, password, role, university):
        requestJson = self.__recvThread.generateBaseCommand("user_create")
        requestJson["email"] = email;
        requestJson["nick"] = nick;
        requestJson["role"] = role;
        requestJson["password"] = password;
        requestJson["university"] = university;
        cmd_m = self.__recvThread.addToSendQueue(requestJson);
        return self.__looper(cmd_m);

    def scoreboard(self):
        requestJson = self.__recvThread.generateBaseCommand("scoreboard")
        requestJson["page"] = 0;
        requestJson["onpage"] = 10;
        cmd_m = self.__recvThread.addToSendQueue(requestJson);
        return self.__looper(cmd_m);