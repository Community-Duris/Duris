-- create polls tables if they don't exist
-- run with: mysql -u duris -p duris < migrations/create_polls_tables.sql

CREATE TABLE IF NOT EXISTS `polls` (
  `id` int(11) NOT NULL auto_increment,
  `question` varchar(512) NOT NULL,
  `created_by` varchar(32) NOT NULL,
  `created_at` int(11) NOT NULL default '0',
  `expires_at` int(11) NOT NULL default '0',
  `is_active` tinyint(1) NOT NULL default '1',
  `multi_select` tinyint(1) NOT NULL default '0',
  `max_choices` int(11) NOT NULL default '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `poll_options` (
  `id` int(11) NOT NULL auto_increment,
  `poll_id` int(11) NOT NULL,
  `option_num` int(11) NOT NULL,
  `option_text` varchar(256) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `poll_id` (`poll_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `poll_votes` (
  `id` int(11) NOT NULL auto_increment,
  `poll_id` int(11) NOT NULL,
  `account_name` varchar(64) NOT NULL,
  `option_id` int(11) NOT NULL,
  `voted_at` int(11) NOT NULL default '0',
  `char_name` varchar(32) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_vote` (`poll_id`, `account_name`, `option_id`),
  KEY `poll_id` (`poll_id`),
  KEY `account_name` (`account_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
