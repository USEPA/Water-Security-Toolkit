import inp2svg

sInp = "../../../../examples/networks/Net2.inp"
svg = inp2svg.inp2svg(sInp)
svg.writeFile(output_file_name="Net2.html")

sInp = "../../../../examples/networks/Net3.inp"
svg = inp2svg.inp2svg(sInp)
svg.writeFile(output_file_name="Net3.html")

sInp = "../../../../examples/networks/Net6.inp"
svg = inp2svg.inp2svg(sInp)
svg.writeFile(output_file_name="Net6.html")

sInp = "../../../../examples/networks/BWSN_Network_1.inp"
svg = inp2svg.inp2svg(sInp)
svg.writeFile(output_file_name="BWSN_Network_1.html")

sInp = "../../../../examples/networks/BWSN_Network_2.inp"
svg = inp2svg.inp2svg(sInp)
svg.writeFile(output_file_name="BWSN_Network_2.html")
















