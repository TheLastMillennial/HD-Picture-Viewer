# ----------------------------
# Makefile Options
# ----------------------------

NAME = HDPICV
ICON = icon.png
DESCRIPTION = "High Definition Picture Viewer 5                 By TLM"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)