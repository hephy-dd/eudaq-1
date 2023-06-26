CMAKE flags for build
adjust "PYTHON_EXECUTABLE" to local miniconda installation

cmake -D PYTHON_EXECUTABLE=~/dev/miniconda3/bin/python -D EUDAQ_BUILD_PYTHON=ON -D USER_MONOPIX2_BUILD=ON ..

