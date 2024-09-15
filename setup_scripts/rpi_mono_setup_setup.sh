unzip -o cc3dsfs_linux_pi32.zip
unzip -o cc3dsfs_linux_pi64.zip
EXTRACTION_FOLDER="cc3dsfs_linux_pi"
RPI_SETUP_FOLDER="rpi_setup"
mkdir -p ${RPI_SETUP_FOLDER}/files/Desktop/cc3dsfs_versions
mkdir -p ${RPI_SETUP_FOLDER}/files/usr/lib/udev/rules.d
cp ${EXTRACTION_FOLDER}32/cc3dsfs ${RPI_SETUP_FOLDER}/files/Desktop/cc3dsfs_versions/cc3dsfs_32
cp ${EXTRACTION_FOLDER}64/cc3dsfs ${RPI_SETUP_FOLDER}/files/Desktop/cc3dsfs_versions/cc3dsfs_64
cp -r "${EXTRACTION_FOLDER}32/other licenses" "${RPI_SETUP_FOLDER}/files/Desktop/other licenses"
cp ${EXTRACTION_FOLDER}32/*.rules ${RPI_SETUP_FOLDER}/files/usr/lib/udev/rules.d
cp ${EXTRACTION_FOLDER}32/LICENSE ${RPI_SETUP_FOLDER}/files/Desktop
cp ${EXTRACTION_FOLDER}32/README.md ${RPI_SETUP_FOLDER}/files/Desktop
$(cd ${RPI_SETUP_FOLDER} ; zip -ry cc3dsfs_rpi_mono_setup.zip ./*)
cp ${RPI_SETUP_FOLDER}/cc3dsfs_rpi_mono_setup.zip .
