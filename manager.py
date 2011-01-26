#! /usr/bin/env python
# -*- encoding: latin-1 -*-

import sys,os,string,socket

from marshall import *

FSROOT = os.path.join(os.getcwd(), "fsroot")

SERVER_SOCK = os.path.join(FSROOT, "tmp/profiled.socket")

class Manager:
    def __init__(self):
        print "connect ...",
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.setblocking(1)
        self.server.connect(SERVER_SOCK)
        print "ok"

    def syncreq(self, tag, *data):
        message_send(self.server, tag + ".req", *data)
        rsp = message_recv(self.server)
        if rsp == None:
            return None
        rsp,data = rsp
        assert rsp == tag + ".rsp"
        return data

    def get_profiles(self):
        return self.syncreq("get_profiles")

    def has_profile(self, profile):
        return self.syncreq("has_profile", profile)[0]

    def get_profile(self):
        return self.syncreq("get_profile")[0]

    def set_profile(self, profile):
        return self.syncreq("set_profile", profile)[0]

    def get_keys(self):
        return self.syncreq("get_keys")

    def get_value(self, profile, key):
        return self.syncreq("get_value", profile, key)[0]

    def set_value(self, profile, key, val):
        return self.syncreq("set_value", profile, key,val)[0]

    def get_datatype(self, key):
        return self.syncreq("get_datatype", key)[0]

    def get_values(self, profile):
        return self.syncreq("get_values", profile)

if __name__ == "__main__":

    args = sys.argv[1:]
    args.reverse()

    m = Manager()

    print "-"*64
    for p in m.get_profiles():
        print "PROFILE: %s" % p

        for k,v,t in m.get_values(p):
            print "\t%-32s = %12s (%s)" % (k,v,t)

    print "-"*64
    for i in m.get_keys():
        print "KEY:", repr(i)
        print "\t",m.get_datatype(i)

    print "-"*64

    prof = m.get_profile()
    print "INITIAL = %s" % prof
    while args:
        arg = args.pop()
        if arg.startswith("@"):
            arg = arg[1:]
            print "PROFILE = %s: %s" % (arg, m.set_profile(arg))
        elif '=' in arg:
            k,v = arg.split("=")
            print "SET %s = %s" % (k,v)
            print m.set_value(prof,k,v)
        else:
            print "GET %s : %s" % (arg, m.get_value(prof,arg))
