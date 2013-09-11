CC = gcc

SHAOBJS = sha18.o sha33.o shaibutton.o
ONEWIREOBJS = win32lnk.o ds2480ut.o ownetu.o owllu.o owsesu.o owtrnu.o owerr.o \
	ioutil.o crcutil.o
FILEOBJS = mbappreg.o mbeprom.o mbnvcrc.o mbscrcrc.o mbscrex.o mbshaee.o mbee.o \
	mbnv.o mbscr.o mbscree.o mbsha.o owfile.o owpgrw.o owprgm.o rawmem.o owcache.o

all: debit debitvm initcopr initcoprvm initrov initrovvm
nonvm: debit initcopr initrov
vm: debitvm initcoprvm initrovvm

debit: shadebitdemo.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -g -o $@ shadebitdemo.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

debitvm: shadebitdemo.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -DCOPRVM -g -o $@ shadebitdemo.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

initcopr: shainitcopr.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -g -o $@ shainitcopr.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

initcoprvm: shainitcopr.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -DCOPRVM -g -o $@ shainitcopr.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

initrov: shainitrov.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -g -o $@ shainitrov.c shadebit.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

initrovvm: shainitrov.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)
	$(CC) -DCOPRVM -g -o $@ shainitrov.c shadebitvm.o $(SHAOBJS) $(FILEOBJS) $(ONEWIREOBJS)

%.o: %.c
	$(CC) -c $<

clean:
	rm *.o *.exe


