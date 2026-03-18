/**
 * @file
 * A quick, but inflexible extension of GRAPHX for displaying graphics.
 *
 * @authors TheLastMillennial
 */

extern "C" {
	/**
	 * Scales an unclipped transparent sprite 160x120 sprite to 320x240 at XY (0,0).
	 * Transparent Index must be at 2.
	 *
	 * @param[in] sprite Pointer to an initialized sprite structure.
	 */
	void hdl_HalfResSprite_NoClip(const gfx_sprite_t *sprite);
}

