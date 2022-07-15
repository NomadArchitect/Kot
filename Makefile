build:
	bash ./Build/build.sh

run:
	bash ./Build/run.sh

deps-llvm:
	wget https://apt.llvm.org/llvm.sh
	chmod +x llvm.sh
	sudo ./llvm.sh 14 all

deps-debian: deps-llvm
	sudo apt update
	sudo apt install nasm xorriso build-essential qemu-system-x86 -y

all-debian: deps-debian build

.PHONY: build run llvm deps-debian
