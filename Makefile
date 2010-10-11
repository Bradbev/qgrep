dist:
	rm -rf dist
	mkdir dist
	make -C src/cpp dist
	cp src/py/*.py ./dist
	chmod +x dist/igrep.py

.PHONY: dist
