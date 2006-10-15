all: tests compare

tests:
	@echo "starting testsuite"
	@csh AutoExec

compare:
	CygTestLogs=$(CygTestLogs) csh AutoCompare

clean:
	find . -name "*.DeTok" -a ! -name "*.OldTkzr.DeTok" -exec rm -f \{\} \;
	find . -name "*.Log" -a ! -name "*.OldTkzr.Log" -exec rm -f \{\} \;
	find . -name "*.fc" -a ! -name "*.OldTkzr.fc" -exec rm -f \{\} \;
	find . -name "*.fl" -exec rm -f \{\} \;
	find . -name "*.RomHdr" -exec rm -f \{\} \;
	find . -name "*.P" -exec rm -f \{\} \;
	find . -name "*.missing" -exec rm -f \{\} \;

distclean: clean
	rm -f toke detok romheaders
	rm -f AutoComp.Log.*
	rm -f AutoComp.TKdiffs.*

