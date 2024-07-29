cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bachelor-project_dect-nr-plus-harq)

target_sources(app PRIVATE src/main.c src/dect_mac_phy_handler.c src/dect_mac_phy_handler_cb.c src/dect_mac_phy_handler_queue.c src/dect_mac_harq.c)
