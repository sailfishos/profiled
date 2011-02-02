#! /usr/bin/env python

# =============================================================================
# This file is part of profile-qt
#
# Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
#
# Contact: Sakari Poussa <sakari.poussa@nokia.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, 
# this list of conditions and the following disclaimer. Redistributions in 
# binary form must reproduce the above copyright notice, this list of 
# conditions and the following disclaimer in the documentation  and/or 
# other materials provided with the distribution.
#
# Neither the name of Nokia Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
# OF THE POSSIBILITY OF SUCH DAMAGE.
# =============================================================================

import sys,os,socket,select
from marshall import *

ROOT_PREFIX = os.path.join(os.getcwd(), "fsroot")
SERVER_SOCK = ROOT_PREFIX + "/tmp/profiled.socket"
TMP_DIR     = ROOT_PREFIX + "/tmp"
CONFIG_DIR  = ROOT_PREFIX + "/etc/profiles"
VALUES_DIR  = ROOT_PREFIX + "/var/profiles"
PROFILE_INI = os.path.join(VALUES_DIR, "values.ini")
PROFILE_TAG = os.path.join(VALUES_DIR, "current")

OVERRIDE = 'override'
FALLBACK = 'fallback'
DATATYPE = 'datatype'
SPECIALS = (OVERRIDE, FALLBACK, DATATYPE)
BUILTIN  = ('general', 'silent', 'meeting', 'outdoors')

INI_DECODE = (("\\n","\n"),("\\r","\n"),("\\t","\t"),("\\\\","\\"))

class IniFile:
    def __init__(self):
        self.secs = {}

    def add_section(self, name):
        try: return self.secs[name]
        except KeyError: pass
        self.secs[name] = {}
        return self.secs[name]

    def get_value(self, sec, key, val=""):
        try: return self.secs[sec].get(key,val)
        except KeyError: return val

    def add_value(self, sec, key, val):
        self.add_section(sec)[key] = val

    def save(self, path):
        temp = path + ".tmp"
        back = path + ".bak"
        data = []

        for sec, vals in self.secs.items():
            data.append("\n[%s]\n" % sec)
            for key,val in vals.items():
                val = unicode(val).rstrip()
                for t,f in reversed(INI_DECODE):
                    val = val.replace(f,t)
                data.append("%s=%s\n" % (key,val))

        open(temp, "wb").write(unicode("".join(data)).encode("utf-8"))
        if os.path.exists(back): os.remove(back)
        if os.path.exists(path): os.rename(path, back)
        os.rename(temp, path)

    def load(self, path):
        if os.path.exists(path):
            sec = None
            for s in unicode(open(path, "rb").read(), "utf-8").splitlines():
                if s.startswith("#"):
                    pass
                elif s.startswith("["):
                    sec = s[1:].rsplit(']',1)[0]
                    self.add_section(sec)
                elif sec != None and '=' in s:
                    key,val = map(lambda x:x.strip(), s.split("=",1))
                    for f,t in INI_DECODE:
                        val = val.replace(f,t)
                    self.add_value(sec,key,val)
                elif s.strip():
                    print>>sys.stderr, "IniFile.load: %s\n" % s

class ProfileDaemon:
    def __init__(self):
        self.config  = IniFile()
        self.values  = IniFile()
        self.profile = 'general'
        self.clients = {}

        if os.path.exists(SERVER_SOCK): os.remove(SERVER_SOCK)
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.bind(SERVER_SOCK)
        self.server.listen(7)

    def __datatype(self, key):
        return self.config.get_value(DATATYPE, key, "UNKNOWN")

    def __fallback(self, key):
        return self.config.get_value(FALLBACK, key, None)

    def __override(self, key):
        return self.config.get_value(OVERRIDE, key, None)

    def __config(self, profile, key):
        return self.config.get_value(profile, key, None)

    def __current(self, profile, key):
        return self.values.get_value(profile, key, None)

    def __default(self, profile, key):
        return self.__override(key) or \
               self.__config(profile, key) or \
               self.__fallback(key)

    def __profile(self, profile, key):
        return self.__current(profile, key) or self.__default(profile, key)

    def load(self):
        self.values.load(PROFILE_INI)

        sources = filter(lambda x:x.endswith(".ini"), os.listdir(CONFIG_DIR))
        for source in sources:
            source = os.path.join(CONFIG_DIR, source)
            self.config.load(source)

        if os.path.isfile(PROFILE_TAG):
            self.profile = open(PROFILE_TAG).read().strip()

    def save(self):
        open(PROFILE_TAG,"w").write(self.profile)

        tmp = IniFile()
        for sec,data in self.values.secs.items():
            for key,val in data.items():
                if self.__default(sec,key) != val:
                    tmp.add_value(sec, key, val)
        tmp.save(PROFILE_INI)

    def get_profiles(self):
        res = dict.fromkeys(self.values.secs.keys() + self.config.secs.keys())
        res.update(dict.fromkeys(BUILTIN))
        for sec in SPECIALS: res.pop(sec,None)
        return res.keys()

    def has_profile(self, profile):
        return profile in self.get_profiles()

    def get_profile(self):
        return self.profile

    def set_profile(self, profile):
        if self.has_profile(profile):
            self.profile = profile
        return self.profile

    def get_keys(self):
        res = {}
        for sec,data in self.values.secs.items() + self.config.secs.items():
            if sec != DATATYPE: res.update(data)
        return res.keys()

    def get_values(self, profile):
        profile = profile or self.profile
        dtype   = lambda x:self.get_datatype(x)
        value   = lambda x:self.get_value(profile,x)
        return map(lambda x:(x,value(x),dtype(x)), self.get_keys())

    def get_value(self, profile, key):
        profile = profile or self.profile
        return self.__profile(profile, key)

    def set_value(self, profile, key, val):
        profile = profile or self.profile
        if self.has_profile(profile):
            self.values.add_value(profile, key, val)

    def get_datatype(self, key):
        return self.__datatype(key)

    def handle_request(self, client, req):
        req,args = req
        rsp = req[:-4] + ".rsp"
        dta1,dtan = None,None

        if req == "get_keys.req":
            dtan = self.get_keys()
        elif req == "get_datatype.req":
            dta1 = self.get_datatype(*args)
        elif req == "get_value.req":
            dta1 = self.get_value(*args)
        elif req == "set_value.req":
            dta1 = self.set_value(*args)
            self.save()
        elif req == "has_profile.req":
            dta1 = self.has_profile(*args)
        elif req == "get_profile.req":
            dta1 = self.get_profile(*args)
        elif req == "get_profiles.req":
            dtan = self.get_profiles(*args)
        elif req == "set_profile.req":
            dta1 = self.set_profile(*args)
            self.save()
        elif req == "get_values.req":
            dtan = self.get_values(*args)
        else:
            print "UNKNOWN REQ:", req
            return 1

        if dtan == None: dtan = (dta1,)
        message_send(client, rsp, *dtan)
        return 0

    def serve(self):
        mask = select.poll()
        mask.register(self.server.fileno(), select.POLLIN)

        while True:
            for file,event in mask.poll():
                if file == self.server.fileno():
                    client, addr = self.server.accept()
                    file = client.fileno()
                    print "NEW", file, repr(addr)
                    self.clients[file] = client
                    mask.register(file, select.POLLIN|select.POLLHUP)
                else:
                    client = self.clients[file]
                    close  = False
                    if event & select.POLLIN:
                        req = message_recv(client)
                        close = not req or self.handle_request(client, req)
                    if event & ~select.POLLIN:
                        close = 1
                    if close:
                        print "DEL", file
                        mask.unregister(file)
                        del self.clients[file]
                        client.close()

if __name__ == "__main__":
    if not os.path.exists(CONFIG_DIR):
        os.makedirs(CONFIG_DIR)
    if not os.path.exists(VALUES_DIR):
        os.makedirs(VALUES_DIR)
    if not os.path.exists(TMP_DIR):
        os.makedirs(TMP_DIR)
            
    daemon = ProfileDaemon()
    daemon.load()
    daemon.serve()
    daemon.save()
