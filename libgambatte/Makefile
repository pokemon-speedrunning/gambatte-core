CXX = clang++

CXXFLAGS = -DHAVE_STDINT_H \
	-I./src -I./include -I./../common \
	-Wall -Wextra -fomit-frame-pointer \
	-fno-rtti -pedantic -std=c++17 \
	-fsanitize=fuzzer,address

TARGET = gambatte

SRCS = \
	src/bitmap_font.cpp \
	src/cpu.cpp \
	src/gambatte.cpp \
	src/initstate.cpp \
	src/interrupter.cpp \
	src/interruptrequester.cpp \
	src/loadres.cpp \
	src/memory.cpp \
	src/newstate.cpp \
	src/sound.cpp \
	src/state_fuzzer.cpp \
	src/state_osd_elements.cpp \
	src/statesaver.cpp \
	src/tima.cpp \
	src/video.cpp \
	src/mem/camera.cpp \
	src/mem/cartridge.cpp \
	src/mem/huc3.cpp \
	src/mem/memptrs.cpp \
	src/mem/pakinfo.cpp \
	src/mem/rtc.cpp \
	src/mem/sgb.cpp \
	src/mem/time.cpp \
	src/mem/snes_spc/dsp.cpp \
	src/mem/snes_spc/SNES_SPC.cpp \
	src/mem/snes_spc/SNES_SPC_misc.cpp \
	src/mem/snes_spc/SNES_SPC_state.cpp \
	src/mem/snes_spc/spc.cpp \
	src/mem/snes_spc/SPC_DSP.cpp \
	src/mem/snes_spc/SPC_Filter.cpp \
	src/sound/channel1.cpp \
	src/sound/channel2.cpp \
	src/sound/channel3.cpp \
	src/sound/channel4.cpp \
	src/sound/duty_unit.cpp \
	src/sound/envelope_unit.cpp \
	src/sound/length_counter.cpp \
	src/video/ly_counter.cpp \
	src/video/lyc_irq.cpp \
	src/video/next_m0_time.cpp \
	src/video/ppu.cpp \
	src/video/sprite_mapper.cpp \
	src/file/file.cpp

make: $(SRCS)
	$(CXX) -o $(TARGET) $(SRCS) $(CXXFLAGS)

