//odd numbers are ERRORS (program must quit)
//even numbers are WARNINGS (program can continue)
enum HDPIC_ERR
{
	SUCCESS,
	ERR_UNKOWN,
	WARN_UNKOWN,
	ERR_LOW_RAM,
	WARN_LOW_RAM,
	ERR_PALETTE_NOT_FOUND,
	WARN_PALETTE_NOT_FOUND,
	ERR_IMAGE_NOT_FOUND,
	WARN_IMAGE_NOT_FOUND,

};