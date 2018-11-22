CPP = gcc
CPP_OPTIONS = -g -m32 -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector -fleading-underscore -fno-asynchronous-unwind-tables

all: kernel.bin

clean:
	rm -f *.o *.bin

start.o: start.asm gdt_low.asm idt_low.asm irq_low.asm
	nasm -f aout -o start.o start.asm

utils.o: utils.C utils.H
	$(CPP) $(CPP_OPTIONS) -c -o utils.o utils.C

assert.o: assert.C assert.H
	$(CPP) $(CPP_OPTIONS) -c -o assert.o assert.C


# ==== VARIOUS LOW-LEVEL STUFF =====

gdt.o: gdt.C gdt.H
	$(CPP) $(CPP_OPTIONS) -c -o gdt.o gdt.C

machine.o: machine.C machine.H
	$(CPP) $(CPP_OPTIONS) -c -o machine.o machine.C

machine_low.o: machine_low.asm machine_low.H
	nasm -f aout -o machine_low.o machine_low.asm

# ==== EXCEPTIONS AND INTERRUPTS =====

idt.o: idt.C idt.H
	$(CPP) $(CPP_OPTIONS) -c -o idt.o idt.C

irq.o: irq.C irq.H
	$(CPP) $(CPP_OPTIONS) -c -o irq.o irq.C

exceptions.o: exceptions.C exceptions.H
	$(CPP) $(CPP_OPTIONS) -c -o exceptions.o exceptions.C

interrupts.o: interrupts.C interrupts.H
	$(CPP) $(CPP_OPTIONS) -c -o interrupts.o interrupts.C

# ==== DEVICES =====

console.o: console.C console.H
	$(CPP) $(CPP_OPTIONS) -c -o console.o console.C

simple_timer.o: simple_timer.C simple_timer.H
	$(CPP) $(CPP_OPTIONS) -c -o simple_timer.o simple_timer.C

simple_keyboard.o: simple_keyboard.C simple_keyboard.H
	$(CPP) $(CPP_OPTIONS) -c -o simple_keyboard.o simple_keyboard.C

simple_disk.o: simple_disk.C simple_disk.H
	$(CPP) $(CPP_OPTIONS) -c -o simple_disk.o simple_disk.C

blocking_disk.o: blocking_disk.C simple_disk.H
	$(CPP) $(CPP_OPTIONS) -c -o blocking_disk.o blocking_disk.C

# ==== FILE SYSTEM =====

file.o: file.C file.H
	$(CPP) $(CPP_OPTIONS) -c -o file.o file.C

file_system.o: file_system.C file_system.H simple_disk.H
	$(CPP) $(CPP_OPTIONS) -c -o file_system.o file_system.C

# ==== MY_MEMORY =====

paging_low.o: paging_low.asm paging_low.H
	nasm -f aout -o paging_low.o paging_low.asm

page_table.o: page_table.C page_table.H paging_low.H
	$(CPP) $(CPP_OPTIONS) -c -o page_table.o page_table.C

cont_frame_pool.o: cont_frame_pool.C cont_frame_pool.H
	$(CPP) $(CPP_OPTIONS) -c -o cont_frame_pool.o cont_frame_pool.C

vm_pool.o: vm_pool.C vm_pool.H
	$(CPP) $(CPP_OPTIONS) -c -o vm_pool.o vm_pool.C


# ==== THREADS & SCHEDULING =====

threads_low.o: threads_low.asm threads_low.H
	nasm -f aout -o threads_low.o threads_low.asm

thread.o: thread.C thread.H threads_low.H
	$(CPP) $(CPP_OPTIONS) -c -o thread.o thread.C

scheduler.o: scheduler.C scheduler.H thread.H
	$(CPP) $(CPP_OPTIONS) -c -o scheduler.o scheduler.C

fifo_queue.o: fifo_queue.C fifo_queue.H thread.H
	$(CPP) $(CPP_OPTIONS) -c -o fifo_queue.o fifo_queue.C

fifo_scheduler.o: fifo_scheduler.C fifo_scheduler.H thread.H
	$(CPP) $(CPP_OPTIONS) -c -o fifo_scheduler.o fifo_scheduler.C

# ==== KERNEL MAIN FILE =====

kernel.o: kernel.C machine.H console.H gdt.H idt.H irq.H exceptions.H interrupts.H simple_timer.H frame_pool.H mem_pool.H thread.H simple_disk.H file.H file_system.H
	$(CPP) $(CPP_OPTIONS) -c -o kernel.o kernel.C

kernel.bin: start.o utils.o kernel.o \
   assert.o console.o gdt.o idt.o irq.o exceptions.o \
   interrupts.o simple_timer.o simple_keyboard.o \
   thread.o threads_low.o simple_disk.o blocking_disk.o file.o file_system.o \
   paging_low.o page_table.o cont_frame_pool.o vm_pool.o \
    machine.o machine_low.o \
    scheduler.o fifo_queue.o fifo_scheduler.o
	ld -melf_i386 -T linker.ld -o kernel.bin start.o utils.o kernel.o \
   assert.o console.o gdt.o idt.o irq.o exceptions.o interrupts.o \
   simple_timer.o simple_keyboard.o \
   thread.o threads_low.o simple_disk.o blocking_disk.o file.o file_system.o \
   paging_low.o page_table.o cont_frame_pool.o vm_pool.o \
    machine.o machine_low.o \
    scheduler.o fifo_queue.o fifo_scheduler.o
