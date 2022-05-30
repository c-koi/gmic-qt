.PHONY: all clean

QMS = fr.qm zh.qm

.PRECIOUS: $(patsubst %.qm,%.ts,$(QMS))

all: $(QMS)

%.qm: %.ts
	@../lrelease.sh $<

%.ts: gmic_qt_%.csv
	@echo Generating $@ file from $<
	@./csv2ts.sh -o $@ $<

clean:
	rm -f $(QMS) *.ts
