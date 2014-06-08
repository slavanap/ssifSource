#!/bin/sh
wget -r -k -p -E -l 0 -np --accept-regex="\/$|view=co$|view=log$|html$" -e robots=off -e use_proxy=yes -e http_proxy=cache.msu:3128 http://www.lundman.net/cvs/viewvc.cgi/lundman/llink/libdvdnav-4.1.2/
