#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
PROJECT_NAME := esp-nbiot
#If IOT_SOLUTION_PATH is not defined, use relative path as default value
IOT_SOLUTION_PATH ?= $(abspath $(shell pwd)/../../)
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/../../../
include $(IDF_PATH)/make/project.mk
include $(IOT_SOLUTION_PATH)/components/component_conf.mk