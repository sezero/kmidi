
example_DATA = Axel_Elf.mid Cavatina.mid Vangelis-Chung_Kuo.mid Malaguena.mid \
	Mission_Impossible.mid Take_The_A_Train-Seq_by_Dianne_Lewandowski.mid \
	Georgia.mid Mr_Postman.mid
exampledir = $(kde_datadir)/kmidi

EXTRA_DIST = $(example_DATA)

messages: _translatorinfo.cpp
	mv _translatorinfo.cpp ..
	(cd .. && $(XGETTEXT) *.cpp -o $(podir)/kmidi.pot)
	rm -f ../_translatorinfo.cpp

