CC        = gcc 
XCPPFLAGS = 
CFLAGS    = -g -O2 -Wall -Werror $(XCPPFLAGS) $(DEFS)
LDFLAGS   = 
LIBS      = 
STRIP     = strip
OBJS      = logparser.o
DEST      = logparser

logparser:	$(OBJS) 
	$(CC) $(CFLAGS) -o $(DEST) $(OBJS) $(LDFLAGS) $(LIBS) 

all:	logparser

clean:
	rm -f *~ *.o core a.out make.log \#*\# $(DEST).exe $(DEST) $(OBJS)
