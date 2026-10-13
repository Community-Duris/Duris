void githyanki_weapon(P_char ch)
{
	P_obj sword;
	char strn[256];

	if (GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_SORCERER) ||
	    GET_CLASS(ch, CLASS_PSIONICIST) || GET_CLASS(ch, CLASS_CONJURER) ||
	    GET_CLASS(ch, CLASS_ILLUSIONIST) || GET_CLASS(ch, CLASS_SUMMONER))
	{
		sword = read_object(19, VIRTUAL);
		if (sword)
		{
			snprintf(strn, 256, "staff silverish silver %sz", GET_NAME(ch));
			sword->str_mask = STRUNG_KEYS;
			sword->name = str_dup(strn);

			obj_to_char(sword, ch);
			send_to_char(
				"&+CWith a blinding flash of light, a staff suddenly appears in your hands!\r\n",
				ch);
			act("&+CIn a blinding flash of light, a staff suddenly appears in $n's hands!",
			    FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	else if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_REAVER))
	{
		sword = read_object(418, VIRTUAL);
		if (sword)
		{
			snprintf(strn, 256, "warhammer hammer silverish silver %sz", GET_NAME(ch));
			sword->str_mask = STRUNG_KEYS;
			sword->name = str_dup(strn);

			obj_to_char(sword, ch);
			send_to_char(
				"&+CWith a blinding flash of light, a warhammer suddenly appears in your hands!\r\n",
				ch);
			act("&+CIn a blinding flash of light, a warhammer suddenly appears in $n's hands!",
			    FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	else if (GET_CLASS(ch, CLASS_ANTIPALADIN))
	{
		sword = read_object(18, VIRTUAL);
		if (sword)
		{
			snprintf(strn, 256, "longsword sword long silverish silver %sz",
				 GET_NAME(ch));
			sword->str_mask = STRUNG_KEYS;
			sword->name = str_dup(strn);

			obj_to_char(sword, ch);
			send_to_char(
				"&+CWith a blinding flash of light, a sword suddenly appears in your hands!\r\n",
				ch);
			act("&+CIn a blinding flash of light, a sword suddenly appears in $n's hands!",
			    FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	else
		send_to_char("&+CYou should have gotten a special item.  Let a god know.\r\n", ch);
}
