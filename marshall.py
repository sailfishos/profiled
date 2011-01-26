#! /usr/bin/env python
# -*- encoding: latin-1 -*-
import sys,pickle,struct,cStringIO

def debugf(m):
    #sys.stderr.write(m)
    pass

def message_send(socket, tag, *dta):
    msg = (tag,dta)
    body = cStringIO.StringIO()
    pickle.dump(msg, body)
    body = body.getvalue()

    head = struct.pack(">I", len(body))
    data = head + body

    debugf("TX: %s\n" % repr(msg))

    socket.sendall(data)

def message_recv(socket):
    fmt = ">I"
    cnt = struct.calcsize(fmt)
    msg = socket.recv(cnt)
    if len(msg) != cnt:
        debugf("RX: %s\n" % "no head")
        return None

    cnt = struct.unpack(fmt, msg)[0]
    msg = socket.recv(cnt)
    if len(msg) != cnt:
        debugf("RX: %s\n" % "no body")
        return None

    msg = cStringIO.StringIO(msg)
    msg = pickle.load(msg)
    debugf("RX: %s\n" % repr(msg))
    return msg
