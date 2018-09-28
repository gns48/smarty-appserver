-- optics definition
DROP TABLE IF EXISTS LENSES CASCADE;
CREATE TABLE LENSES(
       lID        SERIAL PRIMARY KEY,
       lVendor    VARCHAR(32),             -- lense vendor (currently -- 'Olympus')
       lClass     VARCHAR(10),             -- 'Premium', 'Standard', etc 
       lMount     VARCHAR(10) NOT NULL,    -- OM, 4/3, micro 4/3
       lName      VARCHAR(128) NOT NULL,   -- lense name
       lIndex     VARCHAR(16),             -- 'Macro', 'II', 'R', etc
       lFocal     integer NOT NULL,        -- Focal length 
       lFocalMax  integer,                 -- Focal length max (for zooms)
       lAppMin    float,                   -- Min aperture
       lApp       float NOT NULL,          -- Max aperture
       lFocusMin  float,                   -- Min focus
       lAngleMin  float,                   -- Min Angle
       lAngleMax  float,                   -- Max Angle
       lWeight    integer);                -- Weight (gramms)
CREATE UNIQUE INDEX lNameIdx ON LENSES(lName, lIndex, lMount);

-- zooms, variable apperture
INSERT INTO LENSES(lClass, lMount, lName, lIndex, lFocal, lFocalMax, lApp, lAppMin) VALUES
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 9-18mm 1:4.0‑5.6',   'ED',      9,  18, 4.0, 5.6),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 12-50mm 1:3.5-6.3',  'ED EZ',  12,  50, 3.5, 6.3),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 14-42mm 1:3.5-5.6',  'ED EZ',  14,  42, 3.5, 5.6),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 14-42mm 1:3.5-5.6',  'II R',   14,  42, 3.5, 5.6),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 14-150mm 1:4.0-5.6', 'ED',     14, 150, 4.0, 5.6),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 40-150mm 1:4.0-5.6', 'ED R',   48, 150, 4.0, 5.6),
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 75-300mm 1:4.8-6.7', 'ED II',  75, 300, 4.8, 6.7),
('Standard', '4/3',       'ZUIKO DIGITAL 9-18mm 1:4.0-5.6',     'ED',      9,  18, 4.0, 5.6),
('Standard', '4/3',       'ZUIKO DIGITAL 14-42mm 1:3.5-5.6',    'ED',     14,  42, 3.5, 5.6),
('Standard', '4/3',       'ZUIKO DIGITAL 18-180mm 1:3.5-6.3',   'ED',     18, 180, 3.5, 6.3),
('Standard', '4/3',       'ZUIKO DIGITAL 70-300mm 1:4.0-5.6',   'ED',     70, 300, 4.0, 5.6),
('Standard', '4/3',       'ZUIKO DIGITAL 40-150mm 1:4.0-5.6',   'ED',     40, 150, 4.0, 5.6),
('Pro',      '4/3',       'ZUIKO DIGITAL 11-22mm 1:2.8-3.5',    '',       11,  22, 2.8, 3.5),
('Pro',      '4/3',       'ZUIKO DIGITAL 12-60mm 1:2.8-4.0',    'ED SWD', 12,  60, 2.8, 4.0),
('Pro',      '4/3',       'ZUIKO DIGITAL 14-54mm 1:2.8-3.5',    'II',     14,  54, 2.8, 3.5), 
('Pro',      '4/3',       'ZUIKO DIGITAL 50-200mm 1.2.8-3.5',   'ED SWD', 50, 200, 2.8, 3.5);

-- zooms, fixed apperture
INSERT INTO LENSES(lClass, lMount, lName, lIndex, lFocal, lFocalMax, lApp) VALUES 
('Pro',      'Micro 4/3', 'M.ZUIKO DIGITAL 12-40mm 1:2.8', 'ED',     12,  40, 2.8),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 7-14mm 1:4.0',    'ED',      7,  14, 4.0),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 14-35mm 1:2.0',   'ED SWD', 14,  35, 2.0),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 35-100mm 1:2.0',  'ED',     35, 100, 2.0),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 90-250mm 1:2.8',  'ED',     90, 250, 2.8);      

-- fixed focal length
INSERT INTO LENSES(lClass, lMount, lName, lIndex, lFocal, lApp) VALUES
('',         'Micro 4/3', 'M.ZUIKO DIGITAL 17mm 1:2.8', 'Pancake',     17, 2.8),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 12mm 1:2.0', 'ED',          12, 2.0),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 17mm 1:1.8', '',            17, 1.8),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 25mm 1:1.8', '',            25, 1.8),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 45mm 1:1.8', '',            45, 1.8),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 60mm 1:2.8', 'ED Macro',    60, 2.8),
('Premium',  'Micro 4/3', 'M.ZUIKO DIGITAL 75mm 1:1.8', 'ED',          75, 1.8),
('Standard', '4/3',       'ZUIKO DIGITAL 25mm 1:2.8',   'Pancake',     25, 2.8),
('Standard', '4/3',       'ZUIKO DIGITAL 35mm 1:3.5',   'Macro',       35, 2.0),
('Pro',      '4/3',       'ZUIKO DIGITAL 8mm 1:3.5',    'ED Fisheye',   8, 3.5),
('Pro',      '4/3',       'ZUIKO DIGITAL 50mm 1:2.0',   'ED Macro',    50, 2.0),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 150mm 1:2.0',  'ED',         150, 2.0),
('Top Pro',  '4/3',       'ZUIKO DIGITAL 300mm 1:2.8',  'ED',         300, 2.8);

--set vendors
UPDATE lenses SET lVendor='Olympus' WHERE lName LIKE '%ZUIKO%';




