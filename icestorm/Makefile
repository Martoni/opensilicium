
TARGET=remoteswitch

default: $(TARGET).bin

$(TARGET)_prog: $(TARGET).bin
	sudo iceprog $(TARGET).bin
	touch $(TARGET) 

$(TARGET).bin: $(TARGET).txt
	icepack $(TARGET).txt $(TARGET).bin

$(TARGET).txt: $(TARGET).blif $(TARGET).pcf
	arachne-pnr -d 1k -p $(TARGET).pcf $(TARGET).blif -o $(TARGET).txt

$(TARGET).blif: $(TARGET).v
	yosys -p "synth_ice40 -blif $(TARGET).blif" $(TARGET).v

clean:
	-rm $(TARGET).bin
	-rm $(TARGET)
	-rm $(TARGET).blif
	-rm $(TARGET).txt
