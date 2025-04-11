CC = gcc
CFLAGS = -Wall -Wextra -g 
LDFLAGS = -pthread -lm

# Define output directories
BIN_DIR = bin
ANALYSIS_DIR = analysis

# Define target binaries
TARGETS = $(BIN_DIR)/master $(BIN_DIR)/view $(BIN_DIR)/player

PVS_LOG = $(ANALYSIS_DIR)/PVS-Studio.log
PVS_REPORT = $(ANALYSIS_DIR)/report.json

# Create bin directory before compilation
all: $(BIN_DIR) $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/master: master.c | $(BIN_DIR)
	$(CC) $(CFLAGS) master.c -o $@ $(LDFLAGS)

$(BIN_DIR)/player: player.c | $(BIN_DIR)
	$(CC) $(CFLAGS) player.c -o $@ $(LDFLAGS)

$(BIN_DIR)/view: view.c | $(BIN_DIR)
	$(CC) $(CFLAGS) view.c -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -rf $(BIN_DIR) $(ANALYSIS_DIR)
	rm -f strace_out compile_commands.json

run: all
	./$(BIN_DIR)/master -d 200 -p ./$(BIN_DIR)/player ./$(BIN_DIR)/player ./$(BIN_DIR)/player -v ./$(BIN_DIR)/view

# Generate compile_commands.json (for PVS)
compile_commands.json:
	bear -- make clean all  # Ensures JSON includes fresh build data

# PVS-Studio Static Analysis
analysis: $(PVS_REPORT)

$(ANALYSIS_DIR):
	mkdir -p $(ANALYSIS_DIR)

$(PVS_REPORT): compile_commands.json $(TARGETS) | $(ANALYSIS_DIR)
	pvs-studio-analyzer analyze --file compile_commands.json -o $(PVS_LOG)
	plog-converter -a GA:1,2 -t json $(PVS_LOG) -o $(PVS_REPORT)

.PHONY: all clean run analysis
