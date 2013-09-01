cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Ensishell)
enable_testing()
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror")

#########
# Gestion des variantes
#########
# Vous devez editer la ligne suivante pour y mettre vos logins
#########
set(VARIANTE_LOGINS login1 login2 login3)

###

list(SORT VARIANTE_LOGINS)
execute_process(COMMAND echo ${VARIANTE_LOGINS} COMMAND sha512sum COMMAND cut -f 1 -d " " OUTPUT_VARIABLE VARIANTE_SHA)
if (${VARIANTE_SHA} MATCHES "dd31ac6f934c6263b07338d7f33e212565d6b99694ce520251d838bdc9c537f9de63e79ba5ecd66a11a0e7b0c980c3631ee4c34bca4c415f32fce0c0a243ada2")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()
execute_process(COMMAND echo ${VARIANTE_SHA} COMMAND tr "a-f" "A-F" COMMAND xargs printf "16 i 10 o %s C %% p" COMMAND dc OUTPUT_VARIABLE VARIANTE_SUJET)
message("Votre variante sera la variante numéro " ${VARIANTE_SUJET})
configure_file (
  src/variante.h.in
  ${CMAKE_SOURCE_DIR}/src/variante.h
)

#####
# Détection de la bibliothèque Readline
#####
find_library(READLINE_LIBRARY NAMES readline libreadline history libhistory) 


#########
# Fin de gestion des variantes
#########

##
# Si vous utilisé plusieurs fichiers, en plus de mem.c, pour votre
# allocatuer il faut les ajouter ici
##
add_executable(ensishell src/readcmd.c src/ensishell.c)
target_link_libraries(ensishell readline history)

##
# Programme de test
##
add_test(UnitShellTests ../tests/allShellTests.rb)

##
# Construction de l'archive
##
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${VARIANTE_LOGINS})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
