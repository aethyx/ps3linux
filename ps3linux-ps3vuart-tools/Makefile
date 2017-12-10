
PS3SM_DIR=ps3sm
PS3SM=$(PS3SM_DIR)/ps3sm

PS3AV_DIR=ps3av
PS3AV=$(PS3AV_DIR)/ps3av

PS3DM_DIR=ps3dm
PS3DM=$(PS3DM_DIR)/ps3dm

all: $(PS3SM) $(PS3AV) $(PS3DM)

.PHONY: $(PS3SM)
$(PS3SM):
	$(MAKE) -C $(PS3SM_DIR)

.PHONY: $(PS3AV)
$(PS3AV):
	$(MAKE) -C $(PS3AV_DIR)

.PHONY: $(PS3DM)
$(PS3DM):
	$(MAKE) -C $(PS3DM_DIR)

.PHONY: clean
clean:
	$(MAKE) -C $(PS3SM_DIR) clean
	$(MAKE) -C $(PS3AV_DIR) clean
	$(MAKE) -C $(PS3DM_DIR) clean
