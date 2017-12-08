.PHONY: all
all:
	@mkdir -p build && cd build && cmake -DCMAKE_CXX_FLAGS="-O3" ../.. && make --no-print-directory
	@cp raytracer.py rayTracer
	@chmod +x rayTracer

.PHONY: clean
clean:
	@rm -rf build rayTracer
