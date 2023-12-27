import os
import sys
import json

def gen_rearrangement():
	pass

def main():
	path = sys.argv[1]
	fil = open(path,'r')
	cfg = json.load(fil)
	print(cfg)

if __name__ == "__main__":
	main()

	