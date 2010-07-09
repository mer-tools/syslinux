/*
 * Run this through the ASL compiler to generate the memdisk AML template
 */
DefinitionBlock ("memdisk.asl", "SSDT", 1, "00000", "00000000", 0)
{
    Scope (_SB)
    {
	Device (MDSK)
	{
	    Name (_HID, EisaId ("PNP0A05"))

		Device (AZNQ)
	    {
		Name (_HID, EisaId ("HPA0001"))
	        Name (_CRS, ResourceTemplate ()
		{
		    Memory32Fixed (ReadWrite, 0xaaaaaaaa, 0xbbbbbbbb)
		    Memory32Fixed (ReadWrite, 0xcccccccc, 0xdddddddd)
		})
	    }
	}
    }
}
