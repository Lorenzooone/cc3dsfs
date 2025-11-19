unzip -o cc3dsfs_linux_pi32.zip
unzip -o cc3dsfs_linux_pi64.zip
EXTRACTION_FOLDER="cc3dsfs_linux_pi"
RPI_SETUP_FOLDER="rpi_setup"
RPI_FINAL_ZIP_NAME="cc3dsfs_rpi_kiosk_setup.zip"
BASE_LOCATION=$(pwd)
MAIN_FILES_RPI_SETUP=${RPI_SETUP_FOLDER}/files/main_files
mkdir -p ${MAIN_FILES_RPI_SETUP}/cc3dsfs_versions
mkdir -p ${RPI_SETUP_FOLDER}/files/usr/lib/udev/rules.d
cp ${EXTRACTION_FOLDER}32/cc3dsfs ${MAIN_FILES_RPI_SETUP}/cc3dsfs_versions/cc3dsfs_32
cp ${EXTRACTION_FOLDER}64/cc3dsfs ${MAIN_FILES_RPI_SETUP}/cc3dsfs_versions/cc3dsfs_64
cp -r "${EXTRACTION_FOLDER}32/other licenses" "${MAIN_FILES_RPI_SETUP}/other licenses"
cp ${EXTRACTION_FOLDER}32/*.rules ${RPI_SETUP_FOLDER}/files/usr/lib/udev/rules.d
cp ${EXTRACTION_FOLDER}32/LICENSE ${MAIN_FILES_RPI_SETUP}
cp ${EXTRACTION_FOLDER}32/README.md ${MAIN_FILES_RPI_SETUP}
$(cd ${RPI_SETUP_FOLDER} ; zip -qry ${BASE_LOCATION}/${RPI_FINAL_ZIP_NAME} ./*)
