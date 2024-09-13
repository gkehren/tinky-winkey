CC = cl
CFLAGS =#/Wall /WX
LDFLAGS = Advapi32.lib Kernel32.lib Psapi.lib User32.lib Wtsapi32.lib advapi32.lib
SRC_DIR = src
OBJ_DIR = obj
TARGETS = svc.exe winkey.exe

all: $(TARGETS)

svc.exe: $(OBJ_DIR)\svc.obj
    $(CC) $(CFLAGS) /Fe$@ $** $(LDFLAGS)

winkey.exe: $(OBJ_DIR)\winkey.obj
    $(CC) $(CFLAGS) /Fe$@ $** $(LDFLAGS)

$(OBJ_DIR)\svc.obj: $(SRC_DIR)\svc.c $(SRC_DIR)\svc.h
    $(CC) $(CFLAGS) /c $(SRC_DIR)\svc.c /Fo$(OBJ_DIR)\svc.obj

$(OBJ_DIR)\winkey.obj: $(SRC_DIR)\winkey.c
    $(CC) $(CFLAGS) /c $(SRC_DIR)\winkey.c /Fo$(OBJ_DIR)\winkey.obj

clean:
    del $(OBJ_DIR)\*.obj *.exe