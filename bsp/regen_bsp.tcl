#xsct script to regenerate the bsp libraries in the case of a  microblaze subsytem change and
#therefore a change to the HDF file (link microblaze to new .hdf)
puts "Regenerating bsp libraries"

set script_path [ file dirname [ file normalize [ info script ] ] ]

puts "setting workspace to $script_path"
setws $script_path

#repo -set <path/to/repo>/xilinx/embeddedsw

#the system.hdf is included for the latest toolflow microblaze subsystem
updatehw -hw hw_0 -newhwspec $script_path/microblaze.hdf

regenbsp -bsp skarab_microblaze_bsp
