qemu-system-x86_64 -no-reboot -no-shutdown -M smm=off -serial stdio -machine q35 -cpu qemu64 -smp 8 -cdrom ../Bin/kot.iso -m 256M -s -S