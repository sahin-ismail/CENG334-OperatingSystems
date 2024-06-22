CC = g++

all: ext2sutils.cpp ext2fs.h
		$(CC) ext2sutils.cpp -o ext2sutils

mount:
		mkdir mnt  
		sudo mount -o loop testimg2 mnt/

umount:
		sudo umount mnt
		rm -rf mnt

clean:
		sudo rm -rf ext2sutils

run:
		./ext2sutils dup testimg2 /home/alice/336/hw1/solution.c /home/son.c

runrm:
		./ext2sutils rm testimg2 /home/alice/336/hw1/solution

runrd:
		./ext2sutils rd testimg2 /home/bob/315/hw1/solution.c /home/elmor/deneme3.c

own:
		sudo chown -R ismail mnt/