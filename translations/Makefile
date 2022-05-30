.PHONY: all clean

TS = $(wildcard *.ts)
QMS = $(patsubst %.ts,%.qm,$(TS))

all: $(QMS)

%.qm: %.ts
	@./lrelease.sh $<

clean:
	rm -f $(QMS)
