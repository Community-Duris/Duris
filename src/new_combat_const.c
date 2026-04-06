#include "prototypes.h"
#include "new_combat.h"

// humanoid info

bodyLocInfo humanoidBodyLocInfo[NUMB_HUMANOID_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{		   "head",0.15, -65,BODY_LOC_OVERALL_VERYHIGH,  8,10,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																									}, // death
	{	   "left eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{	  "right eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{	   "left ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{	  "right ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{		   "neck", 0.10, -70, BODY_LOC_OVERALL_VERYHIGH,  4,  3,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{    "upper torso", 0.95,   0,     BODY_LOC_OVERALL_HIGH, 50, 20,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{    "lower torso",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_HUMANOID_UPPER_LEFT_LEG, BODY_LOC_HUMANOID_UPPER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																								}, // not necessarily dead
	{ "upper left arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                     {BODY_LOC_HUMANOID_LEFT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{ "lower left arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                     {BODY_LOC_HUMANOID_LEFT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{	 "left elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                 {BODY_LOC_HUMANOID_LOWER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{"upper right arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                    {BODY_LOC_HUMANOID_RIGHT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{"lower right arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                    {BODY_LOC_HUMANOID_RIGHT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{    "right elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                {BODY_LOC_HUMANOID_LOWER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{	 "left wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                      {BODY_LOC_HUMANOID_LEFT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{    "right wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                     {BODY_LOC_HUMANOID_RIGHT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{	  "left hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{	 "right hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{ "upper left leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                      {BODY_LOC_HUMANOID_LEFT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower left leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                     {BODY_LOC_HUMANOID_LEFT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"upper right leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                     {BODY_LOC_HUMANOID_RIGHT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{"lower right leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                    {BODY_LOC_HUMANOID_RIGHT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "left knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                 {BODY_LOC_HUMANOID_LOWER_LEFT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	 "right knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                {BODY_LOC_HUMANOID_LOWER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "left foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "right foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "left ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                      {BODY_LOC_HUMANOID_LEFT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{    "right ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                     {BODY_LOC_HUMANOID_RIGHT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{		   "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin},
	{  "left shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                 {BODY_LOC_HUMANOID_UPPER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{ "right shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                {BODY_LOC_HUMANOID_UPPER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder}
};

// four-armed humanoid info

bodyLocInfo fourArmedHumanoidBodyLocInfo[NUMB_FOUR_ARMED_HUMANOID_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{				 "head",0.15, -65,BODY_LOC_OVERALL_VERYHIGH,  8,10,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																																}, // death
	{			 "left eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			"right eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			 "left ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{			"right ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{				 "neck", 0.10, -70, BODY_LOC_OVERALL_VERYHIGH,  4,  3,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{		  "upper torso", 0.95,   0,     BODY_LOC_OVERALL_HIGH, 50, 20,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{		  "lower torso",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_LEG, BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																															}, // not necessarily dead
	{ "upper left upper arm", 0.15, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{ "lower left upper arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{	 "upper left elbow", 0.08, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                      {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_UPPER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{"upper right upper arm", 0.15, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                         {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{"lower right upper arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                         {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{    "upper right elbow", 0.08, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                     {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_UPPER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{	 "upper left wrist", 0.03, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                           {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{    "upper right wrist", 0.03, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{	  "upper left hand", 0.05, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{	 "upper right hand", 0.05, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{	   "upper left leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                                 {BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{	   "lower left leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                                {BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "upper right leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                                {BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{	  "lower right leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                               {BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{			"left knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                            {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{		   "right knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                           {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{			"left foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{		   "right foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{		   "left ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                 {BODY_LOC_FOUR_ARMED_HUMANOID_LEFT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{		  "right ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                {BODY_LOC_FOUR_ARMED_HUMANOID_RIGHT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{				 "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin},
	{  "upper left shoulder", 0.12, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                      {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_UPPER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{ "upper right shoulder", 0.12, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                     {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_UPPER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{ "upper left lower arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15, 10,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{ "lower left lower arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{	 "lower left elbow", 0.08, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                      {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_LOWER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{"upper right lower arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15, 10,                                         {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{"lower right lower arm", 0.15, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                         {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{    "lower right elbow", 0.08, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                     {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_LOWER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{	 "lower left wrist", 0.03, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                           {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_LEFT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{    "lower right wrist", 0.03, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                          {BODY_LOC_FOUR_ARMED_HUMANOID_LOWER_RIGHT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{	  "lower left hand", 0.05, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{	 "lower right hand", 0.05, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																					  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{  "lower left shoulder", 0.12, -20,     BODY_LOC_OVERALL_HIGH,  6,  1,                                      {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_LEFT_LOWER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{ "lower right shoulder", 0.12, -20,     BODY_LOC_OVERALL_HIGH,  6,  1,                                     {BODY_LOC_FOUR_ARMED_HUMANOID_UPPER_RIGHT_LOWER_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder}
};

// quadraped info

bodyLocInfo quadrapedBodyLocInfo[NUMB_QUADRUPED_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{				 "head",0.15, -65,BODY_LOC_OVERALL_HIGH,  8,10,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																													  }, // death
	{			 "left eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			"right eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			 "left ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{			"right ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{				 "neck", 0.10, -70,     BODY_LOC_OVERALL_HIGH,  4,  3,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{		   "front body", 0.95,   0,   BODY_LOC_OVERALL_MIDDLE, 50, 20,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{			"rear body",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_QUADRUPED_UPPER_LEFT_REAR_LEG, BODY_LOC_QUADRUPED_UPPER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																												  }, // not necessarily dead
	{ "upper left front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                           {BODY_LOC_QUADRUPED_LEFT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower left front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                          {BODY_LOC_QUADRUPED_LEFT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"upper right front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                          {BODY_LOC_QUADRUPED_RIGHT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{"lower right front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                         {BODY_LOC_QUADRUPED_RIGHT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "left front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                      {BODY_LOC_QUADRUPED_LOWER_LEFT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	 "right front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                     {BODY_LOC_QUADRUPED_LOWER_RIGHT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "left front foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "right front foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "left front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                           {BODY_LOC_QUADRUPED_LEFT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{    "right front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                          {BODY_LOC_QUADRUPED_RIGHT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{  "upper left rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                            {BODY_LOC_QUADRUPED_LEFT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{  "lower left rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                           {BODY_LOC_QUADRUPED_LEFT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "upper right rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                           {BODY_LOC_QUADRUPED_RIGHT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower right rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                          {BODY_LOC_QUADRUPED_RIGHT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	   "left rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                       {BODY_LOC_QUADRUPED_LOWER_LEFT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "right rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                      {BODY_LOC_QUADRUPED_LOWER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	   "left rear foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "right rear foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "left rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                            {BODY_LOC_QUADRUPED_LEFT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{	 "right rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                           {BODY_LOC_QUADRUPED_RIGHT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{				 "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin}
};

// centaur info

bodyLocInfo centaurBodyLocInfo[NUMB_CENTAUR_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{				 "head",0.15, -65,BODY_LOC_OVERALL_HIGH,  8,10,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																												  }, // death
	{			 "left eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			"right eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			 "left ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{			"right ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{				 "neck", 0.10, -70,     BODY_LOC_OVERALL_HIGH,  4,  3,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{	 "front horse body", 0.95,   0,   BODY_LOC_OVERALL_MIDDLE, 50, 20,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{	  "rear horse body",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_CENTAUR_UPPER_LEFT_REAR_LEG, BODY_LOC_CENTAUR_UPPER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																											  }, // not necessarily dead
	{ "upper left front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                         {BODY_LOC_CENTAUR_LEFT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower left front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                        {BODY_LOC_CENTAUR_LEFT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"upper right front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                        {BODY_LOC_CENTAUR_RIGHT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{"lower right front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                       {BODY_LOC_CENTAUR_RIGHT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "left front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                    {BODY_LOC_CENTAUR_LOWER_LEFT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	 "right front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                   {BODY_LOC_CENTAUR_LOWER_RIGHT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "left front hoof", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "right front hoof", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "left front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                         {BODY_LOC_CENTAUR_LEFT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{    "right front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                        {BODY_LOC_CENTAUR_RIGHT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{  "upper left rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                          {BODY_LOC_CENTAUR_LEFT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{  "lower left rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                         {BODY_LOC_CENTAUR_LEFT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "upper right rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                         {BODY_LOC_CENTAUR_RIGHT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower right rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                        {BODY_LOC_CENTAUR_RIGHT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	   "left rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                     {BODY_LOC_CENTAUR_LOWER_LEFT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "right rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                    {BODY_LOC_CENTAUR_LOWER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	   "left rear hoof", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "right rear hoof", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "left rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                          {BODY_LOC_CENTAUR_LEFT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{	 "right rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                         {BODY_LOC_CENTAUR_RIGHT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{				 "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin},
	{		  "upper torso", 0.95,   0,     BODY_LOC_OVERALL_HIGH, 50, 20,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{		  "lower torso", 0.95,   0,   BODY_LOC_OVERALL_MIDDLE, 45, 20,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedLowerTorso, victLostLowerTorso}, // centaurs can limp along like jackasses with their arms
	{	   "upper left arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                              {BODY_LOC_CENTAUR_LEFT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{	   "lower left arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                              {BODY_LOC_CENTAUR_LEFT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{		   "left elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                          {BODY_LOC_CENTAUR_LOWER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{	  "upper right arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                             {BODY_LOC_CENTAUR_RIGHT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{	  "lower right arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                             {BODY_LOC_CENTAUR_RIGHT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{		  "right elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                         {BODY_LOC_CENTAUR_LOWER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{		   "left wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                               {BODY_LOC_CENTAUR_LEFT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{		  "right wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                              {BODY_LOC_CENTAUR_RIGHT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{			"left hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{		   "right hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{		"left shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                          {BODY_LOC_CENTAUR_UPPER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{	   "right shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                         {BODY_LOC_CENTAUR_UPPER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder}
};

// bird info

bodyLocInfo birdBodyLocInfo[NUMB_BIRD_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{		   "head", 0.15, -65,    BODY_LOC_OVERALL_HIGH,  8, 10,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedHead,     victLostHead}, // death
	{	   "left eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedEye,      victLostEye},
	{	  "right eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedEye,      victLostEye},
	{		   "neck", 0.10, -70,    BODY_LOC_OVERALL_HIGH,  4,  3,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedNeck,     victLostNeck}, // death
	{		   "body", 0.95,   0,  BODY_LOC_OVERALL_MIDDLE, 50, 20,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedBody,     victLostBody}, // death
	{ "upper left leg", 0.45, -25,     BODY_LOC_OVERALL_LOW, 10, 15,       {BODY_LOC_BIRD_LEFT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperLeg, victLostUpperLeg},
	{ "lower left leg", 0.45, -25, BODY_LOC_OVERALL_VERYLOW, 10, 10,      {BODY_LOC_BIRD_LEFT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedLowerLeg, victLostLowerLeg},
	{"upper right leg", 0.45, -25,     BODY_LOC_OVERALL_LOW, 10, 15,      {BODY_LOC_BIRD_RIGHT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperLeg, victLostUpperLeg},
	{"lower right leg", 0.45, -25, BODY_LOC_OVERALL_VERYLOW, 10, 10,     {BODY_LOC_BIRD_RIGHT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedLowerLeg, victLostLowerLeg},
	{	  "left knee", 0.15, -70,     BODY_LOC_OVERALL_LOW,  2,  1,  {BODY_LOC_BIRD_LOWER_LEFT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedKnee,     victLostKnee},
	{	 "right knee", 0.15, -70,     BODY_LOC_OVERALL_LOW,  2,  1, {BODY_LOC_BIRD_LOWER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedKnee,     victLostKnee},
	{	  "left foot", 0.10, -60, BODY_LOC_OVERALL_VERYLOW,  5,  3,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedFoot,     victLostFoot},
	{	 "right foot", 0.10, -60, BODY_LOC_OVERALL_VERYLOW,  5,  3,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedFoot,     victLostFoot},
	{	 "left ankle", 0.08, -80, BODY_LOC_OVERALL_VERYLOW,  2,  1,       {BODY_LOC_BIRD_LEFT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},    victDamagedAnkle,    victLostAnkle},
	{    "right ankle", 0.08, -80, BODY_LOC_OVERALL_VERYLOW,  2,  1,      {BODY_LOC_BIRD_RIGHT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},    victDamagedAnkle,    victLostAnkle},
	{	  "left wing", 0.08, -20,  BODY_LOC_OVERALL_MIDDLE, 35,  3,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedWing,     victLostWing},
	{	 "right wing", 0.08, -20,  BODY_LOC_OVERALL_MIDDLE, 35,  3,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedWing,     victLostWing},
	{		   "beak", 0.08, -80,    BODY_LOC_OVERALL_HIGH,  6,  1,                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedBeak,     victLostBeak}
};

// winged humanoid info

bodyLocInfo wingedHumanoidBodyLocInfo[NUMB_WINGED_HUMANOID_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{		   "head",0.15, -65,BODY_LOC_OVERALL_VERYHIGH,  8,10,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																												  }, // death
	{	   "left eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{	  "right eye", 0.05, -85, BODY_LOC_OVERALL_VERYHIGH,  1,  0,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{	   "left ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{	  "right ear", 0.10, -85, BODY_LOC_OVERALL_VERYHIGH,  2,  0,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{		   "neck", 0.10, -70, BODY_LOC_OVERALL_VERYHIGH,  4,  3,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{    "upper torso", 0.95,   0,     BODY_LOC_OVERALL_HIGH, 50, 20,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{    "lower torso",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_WINGED_HUMANOID_UPPER_LEFT_LEG, BODY_LOC_WINGED_HUMANOID_UPPER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																											  }, // not necessarily dead
	{ "upper left arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                            {BODY_LOC_WINGED_HUMANOID_LEFT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{ "lower left arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                            {BODY_LOC_WINGED_HUMANOID_LEFT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{	 "left elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                        {BODY_LOC_WINGED_HUMANOID_LOWER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{"upper right arm", 0.20, -40,     BODY_LOC_OVERALL_HIGH, 15, 10,                                           {BODY_LOC_WINGED_HUMANOID_RIGHT_ELBOW, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperArm,   victLostUpperArm},
	{"lower right arm", 0.20, -40,   BODY_LOC_OVERALL_MIDDLE, 15,  8,                                           {BODY_LOC_WINGED_HUMANOID_RIGHT_WRIST, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerArm,   victLostLowerArm},
	{    "right elbow", 0.12, -80,   BODY_LOC_OVERALL_MIDDLE,  4,  1,                                       {BODY_LOC_WINGED_HUMANOID_LOWER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedElbow,      victLostElbow},
	{	 "left wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                             {BODY_LOC_WINGED_HUMANOID_LEFT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{    "right wrist", 0.05, -80,   BODY_LOC_OVERALL_MIDDLE,  3,  1,                                            {BODY_LOC_WINGED_HUMANOID_RIGHT_HAND, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedWrist,      victLostWrist},
	{	  "left hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{	 "right hand", 0.07, -70,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedHand,       victLostHand},
	{ "upper left leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                             {BODY_LOC_WINGED_HUMANOID_LEFT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower left leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                            {BODY_LOC_WINGED_HUMANOID_LEFT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"upper right leg", 0.45, -25,      BODY_LOC_OVERALL_LOW, 20, 15,                                            {BODY_LOC_WINGED_HUMANOID_RIGHT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{"lower right leg", 0.45, -25,  BODY_LOC_OVERALL_VERYLOW, 20, 10,                                           {BODY_LOC_WINGED_HUMANOID_RIGHT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "left knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                        {BODY_LOC_WINGED_HUMANOID_LOWER_LEFT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	 "right knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                       {BODY_LOC_WINGED_HUMANOID_LOWER_RIGHT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "left foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "right foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "left ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                             {BODY_LOC_WINGED_HUMANOID_LEFT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{    "right ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                            {BODY_LOC_WINGED_HUMANOID_RIGHT_FOOT, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{		   "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin},
	{  "left shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                        {BODY_LOC_WINGED_HUMANOID_UPPER_LEFT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{ "right shoulder", 0.15, -20, BODY_LOC_OVERALL_VERYHIGH,  6,  1,                                       {BODY_LOC_WINGED_HUMANOID_UPPER_RIGHT_ARM, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedShoulder,   victLostShoulder},
	{	  "left wing", 0.15, -20,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedWing,       victLostWing},
	{	 "right wing", 0.15, -20,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																			  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedWing,       victLostWing}
};

// winged quadraped info

bodyLocInfo wingedQuadrapedBodyLocInfo[NUMB_WINGED_QUADRUPED_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{				 "head",0.15, -65,BODY_LOC_OVERALL_HIGH,  8,10,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																																	}, // death
	{			 "left eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			"right eye", 0.05, -85,     BODY_LOC_OVERALL_HIGH,  1,  0,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			 "left ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{			"right ear", 0.10, -85,     BODY_LOC_OVERALL_HIGH,  2,  0,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEar,        victLostEar},
	{				 "neck", 0.10, -70,     BODY_LOC_OVERALL_HIGH,  4,  3,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedNeck,       victLostNeck}, // death
	{		   "front body", 0.95,   0,   BODY_LOC_OVERALL_MIDDLE, 50, 20,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{			"rear body",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_WINGED_QUADRUPED_UPPER_LEFT_REAR_LEG, BODY_LOC_WINGED_QUADRUPED_UPPER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																																}, // not necessarily dead
	{ "upper left front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                                  {BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower left front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                                 {BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"upper right front leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                                 {BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{"lower right front leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                                {BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	  "left front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                             {BODY_LOC_WINGED_QUADRUPED_LOWER_LEFT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	 "right front knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                            {BODY_LOC_WINGED_QUADRUPED_LOWER_RIGHT_FRONT_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "left front foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "right front foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	 "left front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                  {BODY_LOC_WINGED_QUADRUPED_LEFT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{    "right front ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                 {BODY_LOC_WINGED_QUADRUPED_RIGHT_FRONT_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{  "upper left rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                                   {BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{  "lower left rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                                  {BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "upper right rear leg", 0.20, -30,      BODY_LOC_OVERALL_LOW, 15, 10,                                                  {BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_KNEE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{ "lower right rear leg", 0.20, -30,  BODY_LOC_OVERALL_VERYLOW, 15,  8,                                                 {BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_ANKLE, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	   "left rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                              {BODY_LOC_WINGED_QUADRUPED_LOWER_LEFT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	  "right rear knee", 0.15, -70,      BODY_LOC_OVERALL_LOW,  5,  1,                                             {BODY_LOC_WINGED_QUADRUPED_LOWER_RIGHT_REAR_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedKnee,       victLostKnee},
	{	   "left rear foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "right rear foot", 0.10, -60,  BODY_LOC_OVERALL_VERYLOW,  7,  3,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedFoot,       victLostFoot},
	{	  "left rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                   {BODY_LOC_WINGED_QUADRUPED_LEFT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{	 "right rear ankle", 0.08, -80,  BODY_LOC_OVERALL_VERYLOW,  4,  1,                                                  {BODY_LOC_WINGED_QUADRUPED_RIGHT_REAR_HOOF, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedAnkle,      victLostAnkle},
	{				 "chin", 0.10, -80, BODY_LOC_OVERALL_VERYHIGH,  6,  1,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedChin,       victLostChin},
	{			"left wing", 0.15, -20,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedWing,       victLostWing},
	{		   "right wing", 0.15, -20,   BODY_LOC_OVERALL_MIDDLE,  6,  1,																						  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       victDamagedWing,       victLostWing}
};

// no extremities info

bodyLocInfo noExtremitiesBodyLocInfo[NUMB_NO_EXTREMITIES_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{	 "head", 0.15, -65,   BODY_LOC_OVERALL_HIGH,  8, 10, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedHead, victLostHead}, // death
	{ "left eye", 0.05, -85,   BODY_LOC_OVERALL_HIGH,  1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  victDamagedEye,  victLostEye},
	{"right eye", 0.05, -85,   BODY_LOC_OVERALL_HIGH,  1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  victDamagedEye,  victLostEye},
	{	 "body", 0.95,   0, BODY_LOC_OVERALL_MIDDLE, 50, 20, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedBody, victLostBody}  // death
};

// insectoid info

bodyLocInfo insectoidBodyLocInfo[NUMB_INSECTOID_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{					"head",0.15, -65,BODY_LOC_OVERALL_HIGH,  8,10,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																																																			   }, // death
	{				"left eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			   "right eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{			  "upper body", 0.95,   0,  BODY_LOC_OVERALL_MIDDLE, 50, 20,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{			  "lower body",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_INSECTOID_MIDDLE_LEFT_UPPER_JOINT, BODY_LOC_INSECTOID_MIDDLE_RIGHT_UPPER_JOINT, BODY_LOC_INSECTOID_BACK_LEFT_UPPER_JOINT, BODY_LOC_INSECTOID_BACK_RIGHT_UPPER_JOINT, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																																																		   }, // not necessarily dead

	{    "upper left front leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														   {BODY_LOC_INSECTOID_FRONT_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{    "lower left front leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{  "lower left front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_INSECTOID_FRONT_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{   "upper right front leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														  {BODY_LOC_INSECTOID_FRONT_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{   "lower right front leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "lower right front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_INSECTOID_FRONT_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{  "upper left front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_INSECTOID_FRONT_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{ "upper right front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_INSECTOID_FRONT_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},

	{   "upper left middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														  {BODY_LOC_INSECTOID_MIDDLE_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{   "lower left middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "lower left middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_INSECTOID_MIDDLE_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{  "upper right middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														 {BODY_LOC_INSECTOID_MIDDLE_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{  "lower right middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"lower right middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_INSECTOID_MIDDLE_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{ "upper left middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_INSECTOID_MIDDLE_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{"upper right middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_INSECTOID_MIDDLE_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},

	{	 "upper left rear leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																															{BODY_LOC_INSECTOID_BACK_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{	 "lower left rear leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{   "lower left rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															  {BODY_LOC_INSECTOID_BACK_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{    "upper right rear leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														   {BODY_LOC_INSECTOID_BACK_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{    "lower right rear leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{  "lower right rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_INSECTOID_BACK_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{   "upper left rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															  {BODY_LOC_INSECTOID_BACK_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{  "upper right rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_INSECTOID_BACK_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint}
};

// arachnid info

bodyLocInfo arachnidBodyLocInfo[NUMB_ARACHNID_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{						  "head",0.15, -65,BODY_LOC_OVERALL_HIGH,  8,10,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},victDamagedHead,       victLostHead																																																																				   }, // death
	{					  "left eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{					 "right eye", 0.05, -85,    BODY_LOC_OVERALL_HIGH,  1,  0,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},        victDamagedEye,        victLostEye},
	{					"upper body", 0.95,   0,  BODY_LOC_OVERALL_MIDDLE, 50, 20,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedUpperTorso, victLostUpperTorso}, // death
	{					"lower body",
     0.95,   0,
     BODY_LOC_OVERALL_MIDDLE, 45,
     20, {BODY_LOC_ARACHNID_MIDDLE2_LEFT_UPPER_JOINT, BODY_LOC_ARACHNID_MIDDLE2_RIGHT_UPPER_JOINT, BODY_LOC_ARACHNID_BACK_LEFT_UPPER_JOINT, BODY_LOC_ARACHNID_BACK_RIGHT_UPPER_JOINT, 0, 0, 0, 0, 0, 0},
     victDamagedLowerTorso, victLostLowerTorso																																																																			   }, // not necessarily dead
	{		  "upper left front leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														  {BODY_LOC_ARACHNID_FRONT_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{		  "lower left front leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{		"lower left front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_ARACHNID_FRONT_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{		 "upper right front leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														  {BODY_LOC_ARACHNID_FRONT_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{		 "lower right front leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{	   "lower right front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_ARACHNID_FRONT_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{		"upper left front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_ARACHNID_FRONT_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{	   "upper right front joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_ARACHNID_FRONT_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},

	{   "upper left front middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														 {BODY_LOC_ARACHNID_MIDDLE_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{   "lower left front middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "lower left front middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_ARACHNID_MIDDLE_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{  "upper right front middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														{BODY_LOC_ARACHNID_MIDDLE_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{  "lower right front middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{"lower right front middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														  {BODY_LOC_ARACHNID_MIDDLE_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{ "upper left front middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														   {BODY_LOC_ARACHNID_MIDDLE_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{"upper right front middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														  {BODY_LOC_ARACHNID_MIDDLE_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},

	{    "upper left rear middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														{BODY_LOC_ARACHNID_MIDDLE2_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{    "lower left rear middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{  "lower left rear middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														  {BODY_LOC_ARACHNID_MIDDLE2_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{   "upper right rear middle leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																													   {BODY_LOC_ARACHNID_MIDDLE2_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{   "lower right rear middle leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{ "lower right rear middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														 {BODY_LOC_ARACHNID_MIDDLE2_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{  "upper left rear middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														  {BODY_LOC_ARACHNID_MIDDLE2_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{ "upper right rear middle joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																														 {BODY_LOC_ARACHNID_MIDDLE2_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},

	{		   "upper left rear leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														   {BODY_LOC_ARACHNID_BACK_LEFT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{		   "lower left rear leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{		 "lower left rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_ARACHNID_BACK_LEFT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{		  "upper right rear leg", 0.20, -30,     BODY_LOC_OVERALL_LOW, 15, 10,																														  {BODY_LOC_ARACHNID_BACK_RIGHT_LOWER_JOINT, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedUpperLeg,   victLostUpperLeg},
	{		  "lower right rear leg", 0.20, -30, BODY_LOC_OVERALL_VERYLOW, 15,  8,																																								 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   victDamagedLowerLeg,   victLostLowerLeg},
	{		"lower right rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_ARACHNID_BACK_RIGHT_LOWER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{		 "upper left rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															 {BODY_LOC_ARACHNID_BACK_LEFT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint},
	{		"upper right rear joint", 0.15, -70,     BODY_LOC_OVERALL_LOW,  5,  1,																															{BODY_LOC_ARACHNID_BACK_RIGHT_UPPER_LEG, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedJoint,      victLostJoint}
};

// beholder info

bodyLocInfo beholderBodyLocInfo[NUMB_BEHOLDER_BODY_LOCS] = {
	/* name, hp, hit mod, height loc, rand targ weight, dismember part weight (lb) */
	{    "body", 0.95,   0, BODY_LOC_OVERALL_MIDDLE, 8, 10, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},     victDamagedBody,     victLostBody}, // death
	{"main eye", 0.20, -20, BODY_LOC_OVERALL_MIDDLE, 2,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},      victDamagedEye,      victLostEye},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk},
	{"eyestalk", 0.10, -65, BODY_LOC_OVERALL_MIDDLE, 1,  0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, victDamagedEyestalk, victLostEyestalk}
};

bodyLocInfo *physBodyLocTables[NUMB_PHYS_TYPES] = {humanoidBodyLocInfo,
                                                   fourArmedHumanoidBodyLocInfo,
                                                   quadrapedBodyLocInfo,
                                                   centaurBodyLocInfo,
                                                   birdBodyLocInfo,
                                                   wingedHumanoidBodyLocInfo,
                                                   wingedQuadrapedBodyLocInfo,
                                                   noExtremitiesBodyLocInfo,
                                                   insectoidBodyLocInfo,
                                                   arachnidBodyLocInfo,
                                                   beholderBodyLocInfo};
