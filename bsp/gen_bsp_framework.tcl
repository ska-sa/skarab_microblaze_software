puts "Generating bsp libraries"

set script_path [ file dirname [ file normalize [ info script ] ] ]

#set ::env(PATH) "$::env(PATH):<install path>/SDK/2019.1/gnu/microblaze/lin/bin"

puts "setting workspace to $script_path"
setws $script_path

#the system.hdf is included for the latest toolflow microblaze subsystem
createhw -name hw_0 -hwspec $script_path/microblaze.hdf

#hsi::open_hw_design $script_path/microblaze.hdf

#repo -set <path/to/repo>/xilinx/embeddedsw

createbsp -name skarab_microblaze_bsp -hwproject hw_0 -proc microblaze_0 -os standalone

#exec make -C $script_path/skarab_microblaze_bsp
