
target = arg[1]
compiler = arg[2]
bitness = arg[3]
linker = arg[4]

file_name = target.."_"..compiler.."_"..linker.."_"..bitness

map_to_csv(compiler.."_"..linker.."_"..bitness, file_name..".map", file_name..".csv")
