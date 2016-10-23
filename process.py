vertices = []
indices = [[],[],[]]
nt = []

with open("output","r") as f:
	nv,_ = map(int, f.readline().split())
	for i in range(nv):
		vertices.append(list(map(float, f.readline().split())))
	for i in range(3):
		nt.append(int(f.readline()))
		for j in range(nt[i]):
			indices[i].append(list(map(int, f.readline().split())))

with open("display","w") as f:
	for i in range(3):
		for j in range(nt[i]):
			# f.write("\\addplot3 [fill,gray!"+str(80-i*20)+"] coordinates {")
			f.write("\\addplot3 [] coordinates {")
			for k in range(3):
				f.write("(")
				for l in range(3):
					f.write(str(vertices[indices[i][j][k]][l]))
					if l != 2:
						f.write(",")
				f.write(")")
			f.write("};\n")

