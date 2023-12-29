import os
import sys
import json
from functools import reduce

def gen_rearrangement():
	pass

def main():
	path = sys.argv[1]
	fname=path.strip().split('/')[-1].split('.')[1]
	fil = open(path,'r')
	cfg:dict = json.load(fil)
	print(cfg)
	primaryKeys = ["inputSource","outputSink","module"]
	inputKeys = list(cfg.keys())
	if set(primaryKeys)!=set(inputKeys):
		print(f"Errors in config file\n{path}")
		print(f"Epected keys {primaryKeys}\nGot {inputKeys}")
		return
	modules:dict = cfg["module"]
	moduleNames = list(modules.keys())			#Assuming modules are specified in order
	inputDimensions = modules[moduleNames[0]]["input"]["dimensions"]
	inputBits = modules[moduleNames[0]]["input"]["bitWidth"] * reduce((lambda x, y: x * y), inputDimensions)
	reqInpDimensions = [3,1,inputDimensions[1]*inputDimensions[2]]
	rip = {}
	ripCounter=1
	if(reqInpDimensions!=inputDimensions):
		pass
	else:
		rip[1] = {"dwc":[1024,inputBits],"source":"dma","sink":"IP"}
		ripCounter+=1
	for i in range(1,len(moduleNames)):
		ripCounter += 1
	if modules[moduleNames[-1]]["output"]["dimensions"][1]!=1:
		rip[ripCounter]={"dfc":[modules[moduleNames[-1]]["output"]["dimensions"][1],cfg["outputSink"]["dim"][-1]],"dwc":[24,1024],"source":"IP","sink":"dma"}
	ofil = open(f"{fname}_rip.json",'w')
	json.dump(rip,ofil)
	ofil.close()

if __name__ == "__main__":
	main()

	