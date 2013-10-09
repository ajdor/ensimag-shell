cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Ensishell)
enable_testing()
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror -std=gnu99")

#########
# Gestion des variantes
#########
# Vous devez editer la ligne suivante pour y mettre vos logins
#########
set(VARIANTE_LOGINS login1 login2 login3)

###
list(SORT VARIANTE_LOGINS)
execute_process(COMMAND printf "${VARIANTE_LOGINS}" COMMAND sha512sum COMMAND cut -f 1 -d " " OUTPUT_VARIABLE VARIANTE_SHA)
if (${VARIANTE_SHA} MATCHES "740444f437149a43d9f494c09ceec14f974517a6886da494619e4ed1c6530518b36f4b36615346ee845743ab072be382b3d4308b8a864b3bdd17406c69fc9a43")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()
execute_process(COMMAND printf ${VARIANTE_SHA} COMMAND tr "a-f" "A-F" COMMAND xargs printf "10 o 16 i %s C %% p" COMMAND dc OUTPUT_VARIABLE VARIANTE_SUJET)
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
string(REPLACE ";" "-" LOGINS_SANS_POINTVIRGULE ${VARIANTE_LOGINS})
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${LOGINS_SANS_POINTVIRGULE}$)
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
