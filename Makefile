PY?=python3
dev:
	$(PY) setup.py build_ext -b . -DTRACE_REFCNT
clean:
	rm mocona/*.so
install:
	$(PY) setup.py install --user
test:
	$(PY) -m pytest tests/
