#! /usr/bin/env python3

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

import sys,os

DEST = None

args = sys.argv[1:]
args.reverse()
while args:
    a = args.pop()
    k,v = a[:2],a[2:]
    if k in "-d":
        DEST = v or args.pop()
    else:
        print("Unknown option: %s" % a, file=sys.stderr)
        sys.exit(1)

def dep_filter(deps):
    src, hdr = [], {}

    for dep in deps:
        if dep.endswith(".c"):
            src.append(dep)
        elif dep.startswith("/"):
            continue
        elif not dep in hdr:
            hdr[dep] = None
    hdr = list(hdr.keys())
    hdr.sort(key=lambda x:(x.count("/"), x))
    return src + hdr

for line in sys.stdin.read().replace("\\\n", " ").split("\n"):
    if not ':' in line:
        continue
    dest,srce = line.split(":",1)

    if DEST:
        dest = os.path.basename(dest)
        dest = os.path.join(DEST, dest)

    srce = dep_filter(srce.split())
    print('%s: %s\n' % (dest, " \\\n  ".join(srce)))
