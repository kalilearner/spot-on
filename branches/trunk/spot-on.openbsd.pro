purge.commands = rm -f */*~ *~

QMAKE_EXTRA_TARGETS = purge
SUBDIRS		    = spot-on-gui.openbsd.pro \
		      Kernel/spot-on-kernel.openbsd.pro
TEMPLATE	    = subdirs
