# %: %.sv
# 	verilator --cc $@.sv utils.svh 
# ----------- CONFIGURABLES -----------
VERILATOR      ?= verilator
TOP_MODULE     ?=   chainer_tb
SRC_FILES      =    chainer.sv
TB_FILES       =    chainer_tb.sv
INCLUDES       = -I.
SEEDS_TXT      = covid_0.txt

# ----------- OUTPUTS -----------------
OBJ_DIR        = obj_dir
EXE            = $(OBJ_DIR)/V$(TOP_MODULE)

# ----------- RULES -------------------

all: sim

# Compile and build simulation executable
$(EXE): $(SRC_FILES) $(TB_FILES) utils.svh
	$(VERILATOR) --cc --binary --sv --timing -Wno-WIDTHCONCAT\
	  $(INCLUDES) \
	  $(TB_FILES) $(SRC_FILES) \
	  -top-module $(TOP_MODULE) \
	  -CFLAGS "-g" \
	  --build

# Run the simulation (make sure seeds.txt is present)
sim: $(EXE) $(SEEDS_TXT)
	@echo "Running simulation..."
	./$(EXE)

# Clean up build artifacts
clean:
	rm -rf $(OBJ_DIR) *.log *.vcd

# Remake everything
remake: clean all

.PHONY: all sim clean remake