# Makefile for CandyFolder
VERSION	=	0_1_0
DISTNAME	=	candyfolder-$(VERSION)
SOURCEDISTNAME	=	candyfoldersrc-$(VERSION).zip

all: candywps.dll

candywps.dll:
	-cd c && make

clean:
	-cd o && rm *
	-cd dist_ger && rm -r *
	-cd dist_eng && rm -r *

cleaner:
	-cd o && rm *
	-cd c && rm *.*~
	-cd c && rm *.flc
	-cd include && rm *.*~
	-cd include && rm *.flc
	-cd res && rm *.*~

german:
	-cd dist_ger && rm -r *
	-mkdir dist_ger/Docs
	-cd c && make
	-cp o/candywps.dll ./dist_ger
	-cp o/*.exe ./dist_ger
	-cp o/*.sym ./dist_ger
	-cp install/* ./dist_ger
	-cp Docs/nls/Readme.ger ./dist_ger
	-cp Docs/nls/FILE_ID.DIZ ./dist_ger
	-cp  docs/* ./dist_ger/Docs
	-rm ./dist_ger/docs/*.htm
	-cmd /C copy docs\candy.htm+docs\nls\candy.htm dist_ger\Docs\candy.htm
	-cmd /C copy install\install.cmd+install\nls\install.ger dist_ger\install.cmd
	-cmd /C copy install\uninstal.cmd+install\nls\uninstal.ger dist_ger\uninstal.cmd
	-cd dist_ger && zip -r $(DISTNAME)-ger.zip *

english:
	-cd dist_eng && rm -r *
	-mkdir dist_eng/Docs
	-cd c && make
	-cp o/candywps001.dll ./dist_eng/candywps.dll
	-cp o/*.exe ./dist_eng
	-cp o/*.sym ./dist_eng
	-cp install/* ./dist_eng
	-cp Docs/nls/Readme.eng ./dist_eng
	-cp Docs/nls/FILE_ID.DIZ ./dist_eng
	-cp  docs/* ./dist_eng/Docs
	-rm ./dist_eng/docs/*.htm
	-cmd /C copy docs\candy.htm+docs\nls\candy001.htm dist_eng\Docs\candy.htm
	-cmd /C copy install\install.cmd+install\nls\install.eng dist_eng\install.cmd
	-cmd /C copy install\uninstal.cmd+install\nls\uninstal.eng dist_eng\uninstal.cmd
	-cd dist_eng && zip -r $(DISTNAME)-eng.zip *

distribution:
	make german
	make english

sourcedistribution:
	-make clean
	-make cleaner
	-rm *.zip
	zip  -r $(SOURCEDISTNAME) *
 