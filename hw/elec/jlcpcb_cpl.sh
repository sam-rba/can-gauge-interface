#!/bin/sh

awk '
	BEGIN {
		FS = ","
	}
	{
		ref = $1
		val = $2
		pkg = $3
		posx = $4
		posy = $5
		rot = $6
		side = $7
		print ref FS posx FS posy FS side FS rot
	}' \
| sed \
	-e '0,/Ref/{s/Ref/Designator/}' \
	-e '0,/PosX/{s/PosX/Mid X/}' \
	-e '0,/PosY/{s/PosY/Mid Y/}' \
	-e '0,/Rot/{s/Rot/Rotation/}' \
	-e '0,/Side/{s/Side/Layer/}'
