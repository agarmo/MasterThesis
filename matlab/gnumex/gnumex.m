function varargout = gnumex(varargin)
  % mex bat options file creator for cygwin / mingw gcc
  %
  % Updated for matlab version 6, rationalized perl
  % Facility for added libraries, Fortran engine compilation
  % Matthew Brett 3/5/2000 (DB), 9/7/2001 (PJ), 19/7/02 (RBH)
  %
  % Updated for Matlab 7.4 with some additional changes as detailed
  % in the accompanying file changes.txt Nov. 2007, Kristjan Jonasson
  %
  % Copyright 2000-2007 Free Software Foundation, Inc.
  % This program is free software; you can redistribute it and/or modify
  % it under the terms of the GNU General Public License as published by
  % the Free Software Foundation; either version 2 of the License, or
  % (at your option) any later version.

  % current version number
  VERSION = '2.01';
  [MING CYMN CYGW] = deal(1,2,3);
  [C, F77, G95, GFORTRAN] = deal(1,2,3,4);
  mlv = sscanf(version,'%f',1); % MATLAB VERSION
  if mlv < 5
    error('gnumex will not work for Matlab versions before 5.0');
  end

  if nargin < 1
    action = 'startup';
  else
    action = lower(varargin{1});
  end

  if strcmp(action, 'version')
    varargout = {VERSION};

  elseif strcmp(action, 'about')
    about(VERSION)
    
  elseif strcmp(action, 'startup')
    close all
    % initialises window, controls, sets callbacks
    
    if ~isempty(gnumex('find_cygfig'))
      error('%s\n%s\n', 'Other instances of gnumex are running.' ...
        ,     'Only one instance can be active at the same time.');
      return
    end

    ounits = get(0, 'Units');
    set(0, 'Units', 'points');
    sz = get(0, 'Screensize');
    set(0, 'Units', ounits);

    gm_sz = [475 385];

    % size of gnumex window, in points
    backg1 = [1, 1, 1];
    backg2 = [0.97,0.94,0.84];
    gray = [0.85 0.85 0.85];

    cygfig = figure(... %'Color',gray, ...
      'color', get(0,'defaultuicontrolbackground'),...
      'Name', ['Gnumex (version ' VERSION ')'],...
      'NumberTitle','off',...
      'Units','points', ...
      'Position',[sz(3:4)/2-[200 155] gm_sz], ...
      'Resize', 'off',...
      'Menubar','none', ...
      'Tag','gnumexfig', ...
      'DefaultUiControlFontsize', 10 ...
      );
      %'DefaultUiControlBackgroundColor', gray ...

    % will contain all the GUI objects with useful values
    actfig = [];

    % path browse callback
    if mlv >= 7.4
      pbrowsecb = ['f=get(gcbo,''UserData'');' ...
        'p=uigetpath74(get(f,''UserData''));' ...
        'if ~isempty(p),set(f,''String'',p),end'];
    else
      pbrowsecb = ['f=get(gcbo,''UserData'');' ...
        'p=uigetpath(get(f,''UserData''));' ...
        'if ~isempty(p),set(f,''String'',p),end'];
    end
    % file browse callback
    fbrowsecb = ['f=get(gcbo,''UserData'');' ...
      '[fn p]=uiputfile(get(f, ''String''), get(f,''UserData''));' ...
      'fprintf(''%s'', pwd);'...
      'cd(pwd);if (fn~=0),set(f,''String'',fullfile(p, fn));end'];

    % control vs label vertical offsets
    boxo = 0;  %-14;
    ubrwso = -2; %-16;
    lstbo = -13;  %-16;

    % vertical distances between things
    below_txt = 26;
    below_lst = 26;

    % size for buttons, list boxes
    lbl_sz = [203 16];
    box_sz = [178 17];
    butt_sz = [64 20];
    %
    lbl_sz1 = [130 16];
    lbl_sz2 = [151 16];
    lst_sz = [109 32];
    lst_sz1 = [56 32];
    txt_sz = [456 17];
    
    % horizontal starting points
    butt_r = 8;
    lbl_st = 11;
    box_st = lbl_st + lbl_sz(1);
    butt_st = gm_sz(1)-butt_sz(1)-butt_r;
    %
    lst_st  = lbl_st + lbl_sz1(1);
    lbl_st1 = lst_st + lst_sz(1) + 10;
    lst_st1 = lbl_st1 + lbl_sz2(1);

    % vertical position of current landmark (starts as top of screen)
    landm = gm_sz(2);

    % mingw binary path stuff
    landm = landm - 27; % set landmark to self
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm lbl_sz], ...
      'String','MinGW root directory (may be blank):', ...
      'Style','text', ...
      'Tag','labelmingw');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[box_st landm+boxo box_sz], ...
      'BackgroundColor',backg1, ...
      'Style','edit', ...
      'UserData', 'Select gcc directory',...
      'Tag','editmingw');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+ubrwso butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse mingw');
    actfig = [actfig b];

    % gcc binary path stuff   
    landm = landm - below_txt;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm lbl_sz], ...
      'String','Cygwin root directory (or blank):', ...
      'Style','text', ...
      'Tag','labelcygwin');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[box_st landm+boxo box_sz], ...
      'Style','edit', ...
      'UserData', 'Select gcc directory',...
      'Tag','editcygwin');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+ubrwso butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse cygwin');
    actfig = [actfig b];

    % g95 folder
    landm = landm - below_txt;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm lbl_sz], ...
      'String','Path to g95.exe (or blank):', ...
      'Style','text', ...
      'Tag','labelg95');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[box_st landm+boxo box_sz], ...
      'Style','edit', ...
      'UserData', 'Select g95 directory',...
      'Tag','editg95');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+ubrwso butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse g95');
    actfig = [actfig b];

    % gfortran folder
    landm = landm - below_txt;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm lbl_sz], ...
      'String','Path to gfortran.exe (or blank):', ...
      'Style','text', ...
      'Tag','labelgfortran');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[box_st landm+boxo box_sz], ...
      'Style','edit', ...
      'UserData', 'Select gfortran directory',...
      'Tag','editgfortran');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+ubrwso butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse gfortran');
    actfig = [actfig b];

    % path to these utilities
    landm = landm - below_txt;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm lbl_sz], ...
      'String','Path to gnumex utilities (where gnumex.m is):', ...
      'Style','text', ...
      'Tag','labelgnumex');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[box_st landm+boxo box_sz], ...
      'Style','edit', ...
      'UserData', 'Select directory containing gnumex files',...
      'Tag','editgnumex');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+ubrwso butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse gnumex');
    actfig = [actfig b];

    algn = 'left';
    % select menu for mingw,cygwin,mno-cygwin linking
    landm = landm - below_txt-15;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment',algn, ...
      'Position',[lbl_st landm lbl_sz1], ...
      'String','Environment / linking type: ', ...
      'Style','text', ...
      'Tag','labelmingwf');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[lst_st landm+lstbo lst_sz], ...
      'BackgroundColor',backg2, ...
      'String',['MinGW              ' ...
      ;         'Cygwin -mno-cygwin ' ...
      ;         'Cygwin  (gcc 3.2)  '], ...
      'Style','popupmenu', ...
      'Tag','popmingwf', ...
      'Value',1);
    actfig = [actfig b];

    % mex file or engine exe creation
    %landm = landm - below_lst;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment',algn, ...
      'Position',[lbl_st1 landm lbl_sz2], ...
      'String','Generate mex dll or engine exe?', ...
      'Style','text', ...
      'Tag','labelmexf');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg2, ...
      'Position',[lst_st1 landm+lstbo lst_sz1], ...
      'String',['Mex   ';'Engine'], ...
      'Style','popupmenu', ...
      'Tag','popmexf', ...
      'Value',1);
    actfig = [actfig b];

    % language for compile
    landm = landm - below_lst;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment',algn, ...
      'Position',[lbl_st landm lbl_sz1], ...
      'String','Language for compilation: ', ...
      'Style','text', ...
      'Tag','labellang');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[lst_st landm+lstbo lst_sz], ...
      'BackgroundColor',backg2, ...
      'String',['C / C++              ' ...
      ;         'Fortran 77           ' ...
      ;         'Fortran 95 (g95)     ' ...
      ;         'Fortran 95 (gfortran)'], ...
      'Style','popupmenu', ...
      'Tag','poplang', ...
      'Enable', 'on',...
      'Value',1);
    actfig = [actfig b];

    % precompiled libraries flag
    %landm = landm - below_lst;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment',algn, ...
      'Position',[lbl_st1 landm lbl_sz2], ...
      'String','Add stub (yes if Windows 95/98)? ', ...
      'Style','text', ...
      'Tag','labelstub');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg2, ...
      'Position',[lst_st1 landm+lstbo lst_sz1], ...
      'String',strvcat('No','Yes'), ...
      'Style','popupmenu', ...
      'Tag','popstub', ...
      'Value',1);
    actfig = [actfig b];

    % Processor to compile for
    landm = landm - below_lst;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment',algn, ...
      'Position',[lbl_st landm lbl_sz1], ...
      'String','Optimization level:', ...
      'Style','text', ...
      'Tag','labelscpu');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[lst_st landm+lstbo lst_sz], ...
      'BackgroundColor',backg2, ...
      'String',strvcat( ...
        '-O0 (no optimization)','-O1','-O2','-O3','-O3 -mtune=native'), ...
      'Style','popupmenu', ...
      'Tag','popcpu', ...
      'Value',1);
    actfig = [actfig b];

    % precompiled libraries directory
    landm = landm - below_txt - 21;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm+15 lbl_sz], ...
      'String','Path for libraries and .def files:', ...
      'Style','text', ...
      'Tag','labelprecomdir');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm+boxo txt_sz], ...
      'Style','edit', ...
      'UserData', 'Select directory for precompiled libraries',...
      'Tag','editprecomdir');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st landm+boxo-23 butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack', pbrowsecb, ...
      'Tag','Browse precomdir');
    actfig = [actfig b];
    
    % name for options file
    landm = landm - below_lst - 25;
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm+15 lbl_sz], ...
      'String','Mex options file to create:', ...
      'Style','text', ...
      'Tag','labeloptsname');
    b = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'BackgroundColor',backg1, ...
      'HorizontalAlignment','left', ...
      'Position',[lbl_st landm+boxo txt_sz], ...
      'Style','edit', ...
      'UserData', 'Select file name for opts bat file',...
      'Tag','editoptsname');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st, landm+boxo-23, butt_sz], ...
      'String','Browse', ...
      'UserData', b, ...
      'CallBack',fbrowsecb, ...
      'Tag','Browse optfile');
    actfig = [actfig b];

    % buttons
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st-butt_sz(1)-30-butt_r-5, butt_r, butt_sz+[30 0]], ...
      'String','Make options file', ...
      'UserData', b, ...
      'CallBack', 'gnumex(''makeopt'')',...
      'Tag','MakeOpts');
    c = uicontrol('Parent',cygfig, ...
      'Units','points', ...
      'Position',[butt_st, butt_r, butt_sz], ...
      'String','Exit', ...
      'UserData', b, ...
      'Callback','gnumex(''exitmaybe'')',...
      'Tag','ExitMaybe');

    % menus
    gnumexdir = findmfile('gnumex');
    b = uimenu('Parent',cygfig, ...
      'Label','&File', ...
      'Tag','Filemenu');
    uimenu('Parent',b, ...
      'Label','&Make opts file',...
      'CallBack', 'gnumex(''makeopt'')',...
      'Tag','makeopt');
    uimenu('Parent',b, ...
      'Label','&Load config',...
      'UserData',fullfile(pwd, 'gnumexcfg.mat'),...
      'CallBack', 'gnumex(''menuloadconfig'')',...
      'Tag','loadconfig');
    uimenu('Parent',b, ...
      'Label','&Save config',...
      'CallBack', 'gnumex(''menusaveconfig'')',...
      'Tag','saveconfig');
    uimenu('Parent',b, ...
      'Label','&Default config',...
      'CallBack', 'gnumex(''menudefaults'')',...
      'Tag','defaultconfig');
    uimenu('Parent',b, ...
      'Label','&Exit',...
      'Callback','close(gcf)',...
      'Tag','close');
    c = uimenu('Parent',cygfig,...
      'Label','&Help', ...
      'Tag', 'Helpmenu');
    uimenu('Parent',c,...
      'Label', '&Gnumex version 1.11 web page',...
      'CallBack', 'web(''http://gnumex.sourceforge.net'');');
    uimenu('Parent',c,...
      'Label', 'View &Readme file',...
      'CallBack', ['dos(''notepad ' gnumexdir '\README.'');']);
    uimenu('Parent',c,...
      'Label', 'View &Fortran readme file',...
      'CallBack', ['dos(''notepad ' gnumexdir '\readme-fortran.txt'');'])
    uimenu('Parent',c,...
      'Label', '&About gnumex',...
      'CallBack', 'gnumex(''about'')')
    actfig = [actfig b c];

    % Properties to set for each GUI object
    HDefs = {...
      'String', ...
      'String', ...
      'String', ...
      'String', ...
      'String', ...
      'Value', ...
      'Value', ...
      'Value', ...
      'Value', ...
      'Value', ...
      'String', ...
      'String', ...
      'UserData' ...
      'UserData'};

    pstruct = gnumex('cfg_defaults');
    PDefs = cell2struct([num2cell(actfig') HDefs' fieldnames(pstruct)],...
      {'handle','property','name'},2);
    set(cygfig,'UserData',PDefs);
    gnumex('struct2fig', pstruct, PDefs);

  elseif (strcmp(action, 'cfg_defaults'))
    % load cfg file, otherwise return defaults
    pstruct = gnumex('defaults');
    tmp = gnumex('loadconfig', pstruct.cfgfile); 
    if ~isempty(tmp) % copy only fields from config file that are in default pstruct
      fn = fieldnames(pstruct);
      for i = 1:length(fn)
        fni = fn{i};
        if isfield(tmp,fni), pstruct.(fni) = tmp.(fni); end
      end
    end
    varargout = {pstruct};

  elseif (strcmp(action, 'defaults'))
    % load defaults, return as structure

    % Collect sensible defaults
    % use mingw unless cygwin is installed and mingw is not
    [cygwinpath, mingwpath, gfortpath, g95path] = find_paths();
    if isempty(mingwpath) & ~isempty(cygwinpath)
      environ = CYMN;
    else
      environ = MING;
    end
    
    % use stub
    usestub = 1; % do not add stub

    % options file name
    try
      optdir = prefdir(1);
    catch
      optdir = pwd;
    end
    libdir = fullfile(optdir,'gnumex');
    if ~exist(libdir, 'dir')
      [ok, msg] = mkdir(libdir);
      if ~ok
        errmsg(tit, ['Failed to create directory ' libdir, '(', msg, ')']);
        return
      end
    end

    gnumexdir = findmfile('gnumex');
    pstruct = struct(...
      'mingwpath' ,mingwpath,...
      'cygwinpath',cygwinpath,...
      'g95path'   ,g95path,...
      'gfortpath' ,gfortpath,...
      'gnumexpath',gnumexdir,...
      'environ',   environ,...     % 1=mingw,2=cygwin-mno,3=cygwin)
      'mexf'      ,1,...           % 1=mex, 2=eng
      'lang'      ,1,...           % 1=c/c++, 2=f77, 3=g95, 4=gfortran
      'usestub'   ,1,...           % 1=no, 2=yes
      'optflg'    ,4,...           % -O3
      'precompath',libdir,...
      'optfile'   ,fullfile(optdir, 'mexopts.bat'),...
      'cfgfile'   ,fullfile(optdir, 'gnumexcfg.mat'),...
      'help'      ,0);
    varargout = {pstruct};

  elseif (strcmp(action, 'struct2fig'))
    % put struct into current figure
    if nargin < 2
      error('Expecting a structure to write');
    end
    pstruct = varargin{2};
    if nargin < 3
      PDefs = get(gcf, 'UserData');
    else
      PDefs = varargin{3};
    end
    for i = 1:size(PDefs, 1)
      set(PDefs(i).handle,PDefs(i).property,...
        getfield(pstruct, PDefs(i).name));
    end

  elseif (strcmp(action, 'fig2struct'))
    % put figure data into structure
    if nargin < 2
      PDefs = get(gcf, 'UserData');
      % ckeck if this is valid information
      if ~isstruct(PDefs) | ~isfield(PDefs, 'handle')
        varargout = {[]};
        return
      end
    else
      PDefs = varargin{2};
    end
    pstruct = [];
    for i = 1:size(PDefs, 1)
      pstruct = setfield(pstruct,PDefs(i).name, ...
        get(PDefs(i).handle,PDefs(i).property));
    end
    varargout = {pstruct};

  elseif (strcmp(action, 'find_cygfig'))
    allfigs = findobj('Type','figure');
    alltags = get(allfigs, 'Tag');
    tmp =  strmatch('gnumexfig', alltags);
    if isempty(tmp)
      varargout = {[]};
    else
      varargout = {allfigs(tmp)};
    end

  elseif (strcmp(action, 'fig_def2struct'))
    % if figure is present, returns that, else defaults
    tmp = gnumex('find_cygfig');
    if isempty(tmp)
      pstruct = gnumex('cfg_defaults');
    else
      pstruct = gnumex('fig2struct');
    end
    varargout = {pstruct};

  elseif (strcmp(action, 'loadconfig'))
    % load config file, return as structure
    if nargin < 2
      error('Expecting a config file name')
    end
    varargout = {[]};
    if exist(varargin{2}) == 2
      load(varargin{2}, '-mat')
      if exist('pstruct', 'var')
        varargout = {pstruct};
      else
        errmsg('Did not load gnumex config file',...
          ['File ' varargin{2} ' does not contain valid config settings']);
      end
    end

    %   elseif (strcmp(action, 'saveconfig'))
    %     % save config in structure as file
    %     if nargin < 3
    %       error('Expecting struct and file name')
    %     end
    %     pstruct = varargin{2};
    %     save(varargin{3}, 'pstruct', '-mat');

  elseif (strcmp(action, 'menuloadconfig'))
    % menu callback for load config
    pstruct = gnumex('fig2struct');
    [f p] = uigetfile(pstruct.cfgfile, 'Configuration file to load');
    if ~isempty(f) & ischar(f) & ischar(p)
      fn = fullfile(p, f);
      pstruct = gnumex('loadconfig', fn);
      if ~isempty(pstruct)
        pstruct.cfgfile = fn;
        gnumex('struct2fig', pstruct);
      end
    end

  elseif (strcmp(action, 'menusaveconfig'))
    % menu callback for save config
    pstruct = gnumex('fig2struct');
    [f p] = uiputfile(pstruct.cfgfile, 'Configuration file to save');
    if ~isempty(f) & ischar(f) & ischar(p)
      fn = fullfile(p, f);
      save(fn, 'pstruct', '-mat')
      %gnumex('saveconfig', pstruct, fn);
      pstruct.cfgfile = fn;
      gnumex('struct2fig', pstruct);
    end

  elseif (strcmp(action, 'menudefaults'))
    % menu callback for defaults load
    gnumex('struct2fig', gnumex('defaults'));

  elseif (strcmp(action, 'checkstruct'))
    % checks struct for errors. Gives warnings
    % Returns 0 if errors, 1 if none
    if nargin < 2
      error('Expecting struct as input argument')
    end
    ps = varargin{2};
    env = ps.environ;
    mingwpath = ps.mingwpath;
    if env == MING
      bin = fullfile(ps.mingwpath, 'bin');
      outvs = fdwarn(bin, {'gcc.exe','as.exe','nm.exe','dlltool.exe'});
    else
      bin = fullfile(ps.cygwinpath, 'bin');
      outvs = fdwarn(bin, {'gcc.exe','as.exe','nm.exe','dlltool.exe','cygpath.exe'});
    end
    if outvs{1}  % ok
      if ps.lang == G95
        outvs = fdwarn(ps.g95path, 'g95.exe');
      elseif ps.lang == GFORTRAN
        outvs = fdwarn(ps.gfortpath, 'gfortran.exe');
      end
    end
    outvs = [outvs; fdwarn(ps.gnumexpath, 'linkmex.pl')];
    outvs = [outvs; fdwarn(ps.gnumexpath, 'mexdlltool.exe')];
    outvs = [outvs; fdwarn(ps.gnumexpath, 'uigetpath.dll')];
    outvs = [outvs; fdwarn(ps.gnumexpath, 'shortpath.dll')];
    outvs = [outvs; fdwarn(ps.gnumexpath, 'uigetpath74.mexw32')];
    outvs = [outvs; fdwarn(ps.gnumexpath, 'shortpath74.mexw32')];
    errinds = cat(1, outvs{:, 1});
    varargout = {all(errinds) outvs(~errinds, 2)};
    
  elseif (strcmp(action, 'check-cygwin'))
    % This checks if gcc is <= v3.1 for cygwin linking and
    % whether correct cygwin1.dll is on the Windows path
    ps = varargin{2};
    env = ps.environ;
    bin = fullfile(ps.cygwinpath, 'bin');
    varargout = {0};
    if env == CYGW
      % Check that gcc is ok (has version <= 3.2)
      [err,verss] = dos([fullfile(bin, 'gcc.exe') ' -dumpversion']);
      if err
        warnmsg('warning', 'unable to obtain gcc version');
        return
      else
        while verss(end) < ' ', verss(end) = []; end
        vers = sscanf(verss, '%f', 1);
        if vers > 3.2
          errmsg('GCC VERSION ERROR' ...
            ,    ['The version of gcc is ' verss ' but with Cygwin'] ...
            ,    'linking, Gnumex needs version 3.2 or earlier' ...
            ,    'of gcc. See the file README');
          varargout = {1};
          return
        end
      end
      loc = locate('cygwin1.dll');
      cygfound = lower(fullfile(loc, 'cygwin1.dll'));
      cygcorr = lower(fullfile(ps.cygwinpath, 'bin', 'cygwin1.dll'));
      if isempty(loc)
        warnmsg('warning', 'cygwin1.dll must be on the Windows path'...
          ,                'to run mex files from cygwin');
      elseif ~isequal(cygfound, cygcorr)
        warnmsg('warning', [cygfound ' is first on the Windows path but '] ...
          ,                [cygcorr ' should probably come before it']);
      end
      if ps.environ == CYGW
        warnmsg('warning', 'With Cygwin linking the created mex files will' ...
          ,                'cause Matlab to crash if they are cleared from' ...
          ,                'memory and run again (see the file README)');
      end
    end

  elseif (strcmp(action, 'parsepaths'))
    % parse paths (knock off trailing slash), short paths
    % paths are variables with 'path' in their name
    if nargin < 2
      error('Expecting a struct as argument');
    end
    pstruct = varargin{2};
    fnames = fieldnames(pstruct);
    for i=1:length(fnames)
      if ~isempty(findstr(fnames{i},'path'))
        s = getfield(pstruct, fnames{i});
        s = adddriveletter(s);
        if mlv >= 7.4
          s = shortpath74(s);
        else
          s = shortpath(s);
        end
        while ~isempty(s) & s(end) == '\'
          s = s(1:end-1);
        end
        pstruct = setfield(pstruct, fnames{i}, s);
      end
    end
    varargout = {pstruct};
    
  elseif (strcmp(action, 'makedeffiles'))
    % Use nm.exe to extract the names of mex-, mx- and mat-functions from
    % .lib-files in Matlab's lcc library folder and write them to .def
    % files in the gnumex.m folder. These .def-files can then be used as
    % input to dlltool to create .lib-files in a format apropriate for
    % gcc. From version 7.4 of Matlab no def-files come with matlab, but
    % this provides a workaround.
    nmcmd = varargin{2};
    deffiles = varargin{3};
    % defdir = varargin{4};
    % libs will probably be {'libmex', 'libmx', 'libmat', 'libeng'};
    libdir = [matlabroot '\extern\lib\win32\lcc\'];
    for i=1:length(deffiles)
      [p, lib] = fileparts(deffiles{i});
      [err,list] = dos([nmcmd ' -g --defined-only "' libdir lib '.lib"']);
      if err
        varargout = {0, 'extracting symbols from lib-file failed'};
        return
      end
      tok = textscan(list,'%*s%s%s%*[^\n]');
      code = char(tok{1});
      symbols = tok{2}(code=='T');
      %fid = fopen([defdir '\' deffiles{i}], 'w');
      fid = fopen(deffiles{i}, 'w');
      if fid < 0
        varargout = {0, ['Cannot open file ' deffiles{i} ' for writing']};
        return
      end
      fprintf(fid, 'LIBRARY %s.dll\nEXPORTS\n', lib);
      J = strmatch('_', symbols)';
      for j = J, symbols{j}(1) = ''; end
      for j = 1:length(symbols), fprintf(fid,'%s\n', symbols{j}); end
      fclose(fid);
      ok = 1;
      varargout = {ok,''};
    end

  elseif (strcmp(action, 'exitmaybe'))
    % Exit (possibly after creating options file and/or after confirmation in
    % a dialog (in case options file has not been created yet))
    if isequal(get(gcf,'tag'), 'gnumexfig-optfile-created')
      close(gcf)
    else
      gstr = questdlg({'Options file has not been created'...
        ,              'Do you want to create it now, before exiting?'} ...
        ,             'Create options'...
        ,             'Yes', 'No', 'Cancel' ...
        ,             'No'); % default
      switch gstr
        case 'Yes'
          gnumex('makeopt');
          close(gcf);
        case 'No'
          close(gcf);
        case {'', 'Cancel'}
          return;
      end
    end

  elseif (strcmp(action, 'makeopt'))
    % write struct info to opt file
    % all the above to get here!
    if nargin < 2
      pstruct = gnumex('fig2struct');
    else
      pstruct = varargin{2};
    end
    if nargin < 3
      gui_f = 1; % flag for GUI warnings / confirm boxes
    else
      gui_f = varargin{3};
    end
    %
    % parse paths
    pps = gnumex('parsepaths', pstruct);
    if mlv >= 7.4
      mlr = shortpath74(matlabroot);
    else
      mlr = shortpath(matlabroot);
    end
    
    % get matlab version
    % find perl location
    if mlv < 6  % matlab 5
      perlpath=fullfile(mlr, 'bin', 'perl.exe');
      if ~exist(perlpath,'file')
        perlpath=fullfile(mlr, 'bin','nt','perl.exe');
      end
    else % matlab >= 6
      perlcont = fullfile('sys','perl','win32','bin','perl.exe');
      perlpath = fullfile(mlr, perlcont);
    end

    if ~exist(perlpath,'file')
      perlpath='perl.exe';
      errmsg('Perl not found', ...
        'Failed to find matlab version of perl.exe',...
        'You may need to set path by hand in options file');
    end

    % set libraries to compile for mex/eng creation
    if mlv < 5.2    % < 5.2
      mexdefs = {'matlab.def'};
      engdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      fmexdefs = {'df50mex.def'}; % not checked
      fengdefs = {'dfmx.def', 'dfmat.def', 'dfeng.def'}; % ditto
    elseif mlv < 5.3 % 5.2
      mexdefs = {'matlab.def', 'libmat.def', 'libmatlb.def'};
      engdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      fmexdefs = {'df50mex.def'};
      fengdefs = {'dfmx.def', 'dfmat.def', 'dfeng.def'};
    elseif mlv < 6  % 5.3
      mexdefs = {'matlab.def', 'libmatlbmx.def'};
      engdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      fmexdefs = {'df50mex.def'};
      fengdefs = {'dfmx.def', 'dfmat.def', 'dfeng.def'};
    elseif mlv < 7 % matlab 6
      mexdefs = {'libmx.def', 'libmex.def', 'libmat.def', '_libmatlbmx.def'};
      engdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      fmexdefs = {'libmx.def', 'libmex.def', 'libmat.def', '_libmatlbmx.def'};
      fengdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
    else % matlab 7
      %       mexdefs = {'libmx.def', 'libmex.def', 'libmat.def'};
      %       engdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      %       fmexdefs = {'libmx.def', 'libmex.def', 'libmat.def'};
      %       fengdefs = {'libmx.def', 'libeng.def', 'libmat.def'};
      mexlibs = {'libmx', 'libmex', 'libmat'};
      englibs = {'libmx', 'libeng', 'libmat'};
      mexdefs = strcat(mexlibs,'.def');
      engdefs = strcat(englibs,'.def');
      fmexdefs = mexdefs;
      fengdefs = engdefs;
    end

    [okf msg] = gnumex('checkstruct', pps);
    if ~okf
      errmsg('Gnumex argument check failed', msg{:});
      return
    end
    
    err = gnumex('check-cygwin', pps);
    if err, return, end    

    % create pps.precompath if necessary
    pcp = pps.precompath;
    if ~exist(pcp, 'dir')
      tit = 'Path for libraries problem';
      if exist(pcp, 'file')
        errmsg(tit, [pcp ' exists and is not a directory']);
        return;
      end
      [ok, msg] = mkdir(pcp);
      if ~ok
        errmsg(tit, ['Failed to create directory ' pcp, '(', msg, ')']);
        return
      end
    end
    
    deffiles = unique([mexdefs,engdefs,fmexdefs,fengdefs]);
    if mlv >= 7.4
      tit = 'Problem creating .def files';
      % Create the .def files from the lcc libraries
      path_to_deffiles = pps.precompath;
      full_deffiles = strcat(path_to_deffiles, '\', deffiles); 
      if ~all(paths_exist(full_deffiles))
        % Check that nm.exe can be found
        nm = fullfile(pps.cygwinpath, 'bin', 'nm.exe');
        if ~exist(nm, 'file'), nm = fullfile(pps.mingwpath, 'bin', 'nm.exe'); end
        if ~exist(nm, 'file'), nm = fullfile(pps.gfortpath, 'nm.exe'); end
        if ~exist(nm, 'file'), errmsg(tit, 'Cannot find nm.exe');  return, end
        if gui_f, 
          set(gcf, 'Pointer', 'watch');
        else
          disp('Making .def-files ...');
        end
        [ok, msg] = gnumex('makedeffiles', nm, full_deffiles);
        if ~ok, errmsg(tit, msg); return, end
        if gui_f, set(gcf, 'Pointer', 'arrow'); end
      end
    else
      path_to_deffiles = [mlr '\extern\include'];
    end
    
    % Optimization levels
    optimflags = {...
      '-O0',...
      '-O1',...
      '-O2',...
      '-O3',...
      '-O3 -mtune=native'};

    % get the relevant paths
    switch pps.environ
      case MING, tools_path = fullfile(pps.mingwpath, 'bin');
      otherwise, tools_path = fullfile(pps.cygwinpath, 'bin');
    end
    [pa,fn,ext] = fileparts(tools_path);
    lib_path = fullfile(pa, 'lib');
    if     pps.lang == G95,      compiler_path = pps.g95path;
    elseif pps.lang == GFORTRAN, compiler_path = pps.gfortpath;
    else                         compiler_path = tools_path; end

    % dlltool command needs to be custom thing for Fortran linking
    if pps.lang == C
      dllcmd = [tools_path '\dlltool'];
    else
      dllcmd = [pps.gnumexpath '\mexdlltool -E --as ' tools_path '\as.exe'];
    end

    % check that the right one of mingw/cygwin has been selected
    cpf = exist(fullfile(tools_path,'cygpath.exe'),'file');
    if pps.environ == MING & cpf & ismember(pps.lang, [C, F77])
      if strcmp('Cancel',...
          questdlg(...
          {'cygpath in binary directory',...
          'This appears to clash with selection of mingw links',...
          'Do you want to continue'},...
          'Mingw/Cygwin mismatch','Continue','Cancel','Cancel'))
        return
      end
    elseif ismember(pps.environ, [CYGW, CYMN]) & ~cpf
      errmsg('Gnumex argument check failed:', ...
          'For Cygwin or Cygwin/mingw, need cygpath in binary directory');
      return
    end

    if exist(pps.optfile, 'file') & gui_f
      gstr = questdlg(['Please confirm overwrite of opt file ' pps.optfile], ...
        'Please confirm','Confirm','Cancel','Cancel');
      if strcmp(gstr, 'Cancel')
        return
      end
    end

    % specify libraries
    % eng / mat library root name, library list
    if mlv >= 7
      defs2convert = unique([mexdefs, engdefs]);
      if pps.mexf == 1
        libraries = mexlibs;
      else
        libraries = englibs;
      end
      if pps.lang ~= C, libraries = strcat('f', libraries); end
    else
      if pps.mexf == 1 % mex file
        if pps.lang == C  % c/c++
          defs2convert = mexdefs;
        else % fortran 77 or 95
          defs2convert = fmexdefs;
        end
        librootn = 'mexlib';
        if mlv >= 7, libraries = mexlibs; end
      else % engine file
        if pps.lang == C  % c/c++
          defs2convert = engdefs;
        else % fortran 77 or 95
          defs2convert = fengdefs;
        end
        librootn = 'englib';
      end
    end

    % create mex.def fmex.def and gfortmex.def
    [fid, msg] = fopen([pcp '\mex.def'], 'wt');
    dlgt = 'Precompiled libraries problem: ';   
    if fid < 0, errmsg('Failed to create mex.def', msg); return, end 
    fprintf(fid, '%s\n%s\n', 'EXPORTS', 'mexFunction');
    fclose(fid);
    %
    [fid, msg] = fopen([pcp '\fmex.def'], 'wt');
    if fid < 0, errmsg('Failed to create fmex.def', msg); return, end  
    fprintf(fid, '%s\n%s\n', 'EXPORTS', '_MEXFUNCTION@16=MEXFUNCTION');
    fclose(fid);
    %
    [fid, msg] = fopen([pcp '\gfortmex.def'], 'wt'); % need mixed case for gfortran
    if fid < 0, errmsg('Failed to create gfortmex.def', msg); return, end  
    fprintf(fid, '%s\n%s\n', 'EXPORTS', '_MEXFUNCTION@16=mexfunction');
    fclose(fid);
    
    % create stub file if needed
    if pps.usestub == 2
      fixupc = [pcp '\fixup.c'];
      [fid, msg] = fopen(fixupc, 'w');
      dlgt = 'Stub problem: ';
      if fid < 0, errmsg('Failed to open stub fixup.c', msg); return, end
      fprintf(fid, '%s\n%s\n', 'asm(".section .idata$3");' ...
        ,                      'asm(".long 0,0,0,0, 0,0,0,0");');
      fclose(fid);
      gcc = fullfile(tools_path, 'gcc.exe');
      fixupo = fullfile(pcp, 'fixup.o');
      [err, msg] = dos([gcc ' -c -o ' fixupo ' ' fixupc]);
      if err, errmsg('Fixup fail', 'Could not create fixup.o', msg); return, end
    end
    
    % create libraries
    rewritef = 0;
    INCROOT = [path_to_deffiles '\'];
    for i = 1:length(defs2convert)
      if mlv >= 7
        [pa,fn,ext]=fileparts(defs2convert{i});
        if pps.lang ~= C, fn = ['f' fn]; end
      else
        fn = sprintf('%s%d', librootn, i);
        libraries{i} = ['f' fn '.lib'];
      end
      out_libraries{i} = fullfile(pps.precompath, [fn, '.lib']);
      if ~exist(out_libraries{i}, 'file'),
        rewritef = 1;
      end
    end

    if ~rewritef & gui_f
      s = {'Library remake', '  Use existing  ','  Create again  '};
      gstr = questdlg('Output libraries already exist', s{:}, s{2});
      rewritef = strcmp(gstr, s{3});
    end

    if rewritef
      % make temporary bat file to do the work
      tfn = [tempname '.bat'];
      [tfid msg] = fopen(tfn, 'wt');
      if (tfid == -1)
        errmsg(dlgt, ['Cannot open temporary bat file ' tfn]);
        return
      end
      fp = inline(['fprintf(' num2str(tfid) ', ''%s\n'', x)']);
      fp('@echo off');
      if ~exist(pps.precompath, 'dir')
        fp(['mkdir "' pps.precompath '"']);
      end
      fp(['set PATH=' compiler_path]);
      % if specified directory does not exist, we will try to create it
      % else, if flag file present, delete it
      if exist(out_libraries{1}, 'file')
        delete(out_libraries{1});
      end
      for i = 1:length(defs2convert)
        fp(sprintf('%s --def "%s%s" --output-lib "%s"', ...
          dllcmd, INCROOT, defs2convert{i}, out_libraries{i}));
      end
      fp('echo Done.');
      fclose(tfid);
      if gui_f, set(gcf, 'Pointer', 'watch');
      else disp('Creating libraries ...'); end
      [s m] = dos(tfn);
      if gui_f, set(gcf, 'Pointer', 'arrow'); end
      % check if succeeded, end if no
      if ~exist(out_libraries{1}, 'file')
        errmsg(dlgt, ...
          'Unsuccessful in creating precompiled libraries',...
          ['Returned message was: ' m], ...
          ['Please check ' tfn]);
        return
      else
        delete(tfn);
      end
    end

    [fid msg] = fopen(pstruct.optfile, 'wt');
    if (fid == -1)
      errmsg(dlgt, ['Could not open optfile ' pstruct.optfile]);
      return
    end

    % Added libraries
    % This could be done with the LINKFLAGSPOST variable I think,
    % but I don't know if this is compatible with versions of matlab
    % < 5.3
    add_libs = {};
    if pps.usestub == 2, add_libs = {add_libs{:} fixupo}; end
    if (pps.lang == F77) % fortran 77
      add_libs = {add_libs{:} '-lg2c'};
    end

    % make a little report
    rep = char(gnumex('report', pps, rewritef));

    % inline functin for printing to options .bat file
    fp = inline(['fprintf(' num2str(fid) ', ''%s\n'', x)']);
    
    % at last
    if gui_f
      set(gcf, 'Pointer', 'watch');
    else
      disp(rep);
    end

    % --------------------------------------------------------------

    fp('@echo off');
    fp(['rem ' pps.optfile]);
    fp(['rem Generated by gnumex.m script in ' pps.gnumexpath]);
    fp(['rem gnumex version: ' VERSION]);
    fp('rem Compile and link options used for building MEX etc files with');
    fp('rem the Mingw/Cygwin tools.  Options here are:');
    for i = 1:size(rep, 1)
      fp(['rem ' rep(i,:)]);
    end

    fp(['rem Matlab version ' num2str(mlv)]);
    fp('rem');
    fp(['set MATLAB=' mlr]);
    fp(['set GM_PERLPATH=' perlpath]);
    fp(['set GM_UTIL_PATH=' pps.gnumexpath]);
    fp(['set PATH=' tools_path ';%PATH%']);
    if ~isequal(tools_path, compiler_path)
      fp(['set PATH=' compiler_path ';' tools_path ';%PATH%']);
    end
    fp(['set PATH=%PATH%;C:\Cygwin\usr\local\gfortran\libexec\gcc\i686-pc-cygwin\4.3.0']);
    fp(['set LIBRARY_PATH=' lib_path]);
    fp(['set G95_LIBRARY_PATH=' lib_path]);
    fp('rem');
    fp('rem precompiled library directory and library files');
    fp(['set GM_QLIB_NAME=' pps.precompath]);
    fp('rem');
    fp('rem directory for .def-files');
    fp(['set GM_DEF_PATH=' path_to_deffiles]);
    fp('rem');
    fp('rem Type of file to compile (mex or engine)');
    if pps.mexf == 1, mtype = 'mex'; else mtype = 'eng';end
    fp(['set GM_MEXTYPE=' mtype]);
    switch pps.lang
      case C,        mlang = 'c'; mexdef = 'mex.def';
      case F77,      mlang = 'f77'; mexdef = 'fmex.def';
      case G95,      mlang = 'f95'; mexdef = 'fmex.def';
      case GFORTRAN, mlang = 'f95'; mexdef = 'gfortmex.def';
    end
    mexdef = fullfile(pcp,mexdef);
    fp('rem');
    fp('rem Language for compilation');
    fp(['set GM_MEXLANG=' mlang]);
    fp('rem');
    fp('rem File for exporting mexFunction symbol');
    fp(['set GM_MEXDEF=' mexdef]);
    add_libs = [add_libs strcat('-l',libraries)];
    fp('rem');
    fp(['set GM_ADD_LIBS=' sepcat(add_libs, ' ')]);
    fp('rem');
    fp('rem compiler options; add compiler flags to compflags as desired');
    fp('set NAME_OBJECT=-o');
    
    if mlv > 5.1
      if ismember(pps.lang, [C, F77])
        fp(['set COMPILER=gcc']);
      elseif pps.lang == G95
        fp(['set COMPILER=g95']);
      elseif pps.lang == GFORTRAN
        fp(['set COMPILER=gfortran']);
      else
        error 'Illegal combination of Unix tools and Language';
      end
    else
      fp('rem Need wrapper for compiler to move .o output to .obj');
      fp(['set COMPILER=mexgcc']);
    end

    if pps.environ == CYMN
      c = '-mno-cygwin';
    else
      c = '';
    end
    if pps.lang ~= C % fortran
      % stdcall compile, upper case symbols, no underscore suffix
      c = ['-fno-underscoring ' c];
      if mlv < 7.4, c = ['-mrtd ' c]; end
      if ismember(pps.lang, [F77, G95]), c = ['-fcase-upper ' c]; end
    end
    fp(['set COMPFLAGS=-c -DMATLAB_MEX_FILE ' c]);
    fp(['set OPTIMFLAGS=' optimflags{pps.optflg}]);
    fp('set DEBUGFLAGS=-g');
    if pps.lang == C
      fp(['set CPPCOMPFLAGS=%COMPFLAGS% -x c++ ' c]);
      fp('set CPPOPTIMFLAGS=%OPTIMFLAGS%');
      fp('set CPPDEBUGFLAGS=%DEBUGFLAGS%');
    end
    fp('rem');
    fp('rem NB Library creation commands occur in linker scripts');

    % main linker parameters
    if pps.environ == CYMN  % cygwin/mingw compile
      linkfs = ['-mno-cygwin -mwindows'];
    else                  % mingw or cygwin compile
      linkfs = '';
    end
    linker = '%GM_PERLPATH% %GM_UTIL_PATH%\linkmex.pl';
    if (pps.mexf == 1)    % mexf compile
      if mlv >= 7.1, oext = 'mexw32'; else oext = 'dll'; end
    else                  % engine compile
      oext = 'exe';
    end

    % resource linker parameters
    if ismember(pps.environ, [MING])
      rclinkfs = '';
    else                  % cygwin or cygwin/mingw compile
      rclinkfs = '--unix';
    end

    fp('rem');
    fp('rem Linker parameters');
    fp(['set LINKER=' linker]);
    fp(['set LINKFLAGS=' linkfs]);
    if pps.lang == C % c
      % indicator to linkmex script that this is a c++ file
      % so linker can be set to g++ rather than gcc
      fp(['set CPPLINKFLAGS=GM_ISCPP ' linkfs]);
    end
    fp('set LINKOPTIMFLAGS=-s');
    fp('set LINKDEBUGFLAGS=-g  -Wl,--image-base,0x28000000\n');
    fp(['set LINKFLAGS=' linkfs ' -L' pps.precompath]);
    fp('set LINK_FILE=');
    fp('set LINK_LIB=');
    fp(['set NAME_OUTPUT=-o %OUTDIR%%MEX_NAME%.' oext]);
    fp('rem');
    fp('rem Resource compiler parameters');
    fp(['set RC_COMPILER=%GM_PERLPATH% %GM_UTIL_PATH%\rccompile.pl '...
      rclinkfs ' -o %OUTDIR%mexversion.res']);
    fp('set RC_LINKER=');

    % done
    fclose(fid);
    msg = ['Created opt file ' pstruct.optfile];
    if gui_f
      set(gcf, 'tag', 'gnumexfig-optfile-created');
      set(gcf, 'Pointer', 'arrow');
      m = msgbox(msg, 'Gnumex opt file');
      uiwait(m);
    else
      disp(msg);
    end

    % --------------------------------------------------------------

  elseif (strcmp(action, 'report'))
    % Compiles a little report on current options
    pstruct = varargin{2};
    rewritef = varargin{3};

    switch pstruct.environ
      case MING
        lnk = 'MinGW';
      case CYGW
        lnk = 'Cygwin (cygwin1.dll)';
      case CYMN
        lnk = 'Cygwin -mno-cygwin';
    end
    switch pstruct.mexf
      case 1
        mext = 'Mex (*.dll)';
      case 2
        mext = 'Engine / mat (*.exe)';
    end
    switch rewritef
      case 1
        libcre = 'Libraries regenerated now';
      case 0
        libcre = 'Existing libraries used';
    end
    switch pstruct.lang
      case C
        lang = 'C / C++';
      case F77
        lang = 'Fortran 77';
      case G95
        lang = 'Fortran 95 (g95)';
      case GFORTRAN
        lang = 'Fortran 95 (gfortran)';
    end
    switch pstruct.optflg
      case 1
        optflg = '-O0 (no optimization)';
      case 2
        optflg = '-O1';
      case 3
        optflg = '-O2';
      case 4
        optflg = '-O3 (full optimization)';
      case 5
        optflg = '-O3 -mtune=native';
    end

    varargout = {{...
      ['Gnumex, version ' VERSION],...
      [lnk ' linking'],...
      [mext ' creation'],...
      libcre,...
      ['Language: ' lang],...
      ['Optimization level: ' optflg]}};

  elseif (strcmp(action, 'test'))
    % Rather hackey test script
    % Usage: gnumex('test', cygwinoldpath, testf)
    %   for example: gnumex test
    %   or:          gnumex test c:\cygwin_old
    %   or:          gnumex test c:\cygwin_old 1
    %
    % NOTE: no-cygwin (ctype=3) does not work with old cygwin (the one with gcc
    %       3.2) but cygwin only works with the old cygwin. So to test properly
    %       both versions of cygwin must be present. For testing, it is assumed
    %       that the new version is returned by gnumex('fig_def2struct') below,
    %       and the old cygwin path is pointed to by the command line argument.
    
    % arg - testf
    % non-zero if we are testing output of yprime dll with cygwin
    % This causes matlab to crash and die after a few Cygwin tests on my machine
    
    testf = 0;
    cygwin_old_path = '';
    if nargin >= 3
      testf = varargin{3};
    end
    if nargin >= 2
      cygwin_old_path = varargin{2};
    end
    % File to save results
    flagfile = 'checked.mat';

    % path to file examples
    fortran_mexfun = 'yprime77';
    exroot = fullfile(matlabroot, 'extern', 'examples');
    fexpath = findmfile('gnumex');
    fortran_examples = {fullfile(fexpath, 'examples', [fortran_mexfun '.f']) ...
      ,                 fullfile(fexpath, 'examples', 'yprimef.f')};
    % file examples
    % 2x2 cell matrix, col 1 mex, col 2 eng, row 1 c row 2 fortran
    mexstr{1,1} = {fullfile(exroot, 'mex', 'yprime.c')};
    mexstr{2,1} = fortran_examples;
    mexstr{1,2} = {fullfile(exroot, 'eng_mat', 'engwindemo.c')};
    mexstr{2,2} = {fullfile(fexpath, 'examples', 'fengdemo.f')};

    % c and fortran versions of evals to test (testf = 1)
    mexteststr = {'yprime(1, 1:4)', [fortran_mexfun '(1, 1:4)']};

    % correct result from yprime
    corr_r = [2.0000    8.9685    4.0000   -1.0947];

    % Fill structure
    pstruct = gnumex('fig_def2struct');
    pstruct.precompath = pwd;
    pstruct.optfile = fullfile(pwd,'testopts.bat');

    % delete existing libaries, checked flag file
    %  --- No. don't delete them to make the test run faster!
    % delete *.lib 
    if exist(flagfile, 'file')
      delete(flagfile)
    end

    % test each in turn
    success = cell(3,2,2,2);
    if isempty(cygwin_old_path), CTYPES = [MING,CYMN];
    else                         CTYPES = [MING,CYMN,CYGW]; end
    for ctype = CTYPES
      pstruct.environ = ctype;
      if ~isempty(cygwin_old_path) & ctype == 2
        pstruct.cygwinpath = cygwin_old_path; 
      end
      for lang = 1:2  % c, fortran77
        pstruct.lang = lang;
        clear yprime
        clear(fortran_mexfun)
        for targ = 1:2  % mex, engine
          if targ == 2 & ctype == 2, continue, end
          %                               d  (engine doesn't work with cygwin)
          pstruct.mexf = targ;
          try
            gnumex('makeopt', pstruct, 0);
            mex('-f', pstruct.optfile, mexstr{lang, targ}{:});
            if targ == 1 & (ctype ~= 2 | testf)
              r = eval(mexteststr{lang});
              if any((r - corr_r) > eps)
                error(['Mex gave ' num2str(r)]);
              end
            end
            fprintf('OK\n');
          catch
            fprintf('Failed with %s\n', lasterr);
            success{ctype, 1, lang, targ} = lasterr;
          end
        end
      end
    end

    % delete existing libaries, clear dll
    %  --- No. don't delete them to make the test run faster!
    % delete *.lib
    clear yprime
    clear(fortran_mexfun)

    % save flag file
    save(flagfile, 'success');

    %   elseif (strcmp(action, 'fbrowsecb')) % NEVER CALLED
    %     f=get(gcbo,'UserData');
    %     [fn p]=uiputfile(get(f, 'String'), get(f,'UserData'))
    %     cd(pwd);
    %     if ~isempty(fn),
    %       set(f,'String',fullfile(p, fn));
    %     end

  elseif (strcmp(action, 'usage'))

    [path_opts cmd_opts] = gnumex('command_options');
    fprintf(['Gnumex, version ' VERSION '. '])
    fprintf('Command line options:\n');
    fprintf('%s=[path]\n', path_opts{:});
    fprintf('%s\n', cmd_opts{:, 1});
    fprintf('%s\n', 'usage');

  elseif (strcmp(action, 'command_options'))

    path_opts = {...
      'mingwpath',...
      'cygwinpath',...
      'g95path',...
      'gfortpath',...
      'gnumexpath',...
      'precompath',...
      'optfile',...
      'cfgfile'};
    cmd_opts = {...
      'mingw',     'environ', 1;...
      'no-cygwin', 'environ', 2;...
      'cygwin',    'environ', 3;...
      'mex',       'mexf',    1;...
      'eng',       'mexf',    2;...
      'c',         'lang',    1;...
      'fortran77', 'lang',    2;...
      'g95',       'lang',    3;...
      'gfortran'   'lang',    4;...
      'nostub',    'stub',    1;...
      'stub',      'stub',    2;...
      'O0',        'optflg',  1;...
      'O1',        'optflg',  2;...
      'O2',        'optflg',  3;...
      'O3',        'optflg',  4;...
      'O3nat',     'optflg',  5 ...
      };
    varargout = {path_opts, cmd_opts};

  else % otherwise unrecognized option

    if ~isempty(gnumex('find_cygfig'))
      error('%s\n%s\n', 'Other instances of gnumex are running.' ...
        ,     'Only one instance can be active at the same time.');
      return
    end
    
    % implement a command line option parser
    pstruct = gnumex('fig_def2struct');
    [path_opts cmd_opts] = gnumex('command_options');
    args = varargin;

    % first try path setters (with = in arg)
    args_remaining = logical(ones(size(args)));
    errstr = {};
    for i = 1:length(args)
      arg = args{i};
      for p = path_opts
        if ~isempty(strmatch(p, arg))
          args_remaining(i)=0;
          p = char(p);
          p_st = length(p)+ 2;
          if isempty(strmatch([p '='], arg)) ...
              | length(arg) < p_st
            errstr{end+1} = ['Path syntax is ' p '=[path]'];
          else
            pstruct = setfield(pstruct,...
              p,...
              arg(p_st:end));
          end
          continue;
        end
      end
    end
    args = args(args_remaining);

    % and non path arguments
    [ok_opts opt_inds] = ismember(args, cmd_opts(:,1));

    if ~all(ok_opts)
      errstr{end+1} = sprintf('Unrecognized option: %s\n',args{~ok_opts});
    end
    if ~isempty(errstr)
      gnumex('usage');
      error(sprintf('%s\n', errstr{:}));
    end

    for i = 1:length(args)
      pstruct = setfield(pstruct,...
        cmd_opts{opt_inds(i),2},...
        cmd_opts{opt_inds(i),3});
    end
    gnumex('makeopt', pstruct, 0);
  end
  return


  % subroutines

function varargout = fdwarn(pathname, filename)
  % checks whether path is valid, then for file therein contained
  % returns true if both are true, msg for warning if not
  okf = 1;
  warnmsg = [];
  if ischar(filename), filename = {filename}; end
  if ~exist(pathname, 'dir')
    warnmsg = [pathname ' does not appear to be a valid directory'];
    okf = 0;
  elseif iscell(filename)
    notf = {};
    for i = 1:length(filename)
      fn = filename{i};
      if ~exist(fullfile(pathname,fn),'file')
        notf = {notf{:}, fn}; 
        okf = 0;
      end
    end
    if ~okf
      warnmsg = [pathname ' does not contain: ' sepcat(notf, ', ')];
    end
  end
  varargout = {{okf warnmsg}};
  return

function dir = findmfile(mfilename)
  % returns dir for m file for function specified
  dir = lower(which(mfilename));
  if isempty(dir)
    return
  end
  t = findstr(dir, [filesep mfilename '.m']);
  if isempty(t)
    return
  end
  dir = dir(1:t(1)-1);
  return

function s = sepcat(strs, sep)
  % returns cell array of strings as one char string, separated by sep
  if nargin < 2
    sep = ';';
  end
  if isempty(strs)
    s = '';
    return
  end
  strs = strs(:)';
  strs = [strs; repmat({sep}, 1, length(strs))];
  s = [strs{1:end-1}];
  
function [cygwinpath, mingwpath, gfortpath, g95path] = find_paths();
  % Try to find locations of cygwin and mingw
  % Mark Levedahl suggested this to find Cygwin
  reg_cygwin = 'SOFTWARE\Cygnus Solutions\Cygwin\mounts v2\/';
  reg_mingw = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\MinGW';
  cygwinpath = find_path(reg_cygwin, 'native', 'c:\cygwin');
  mingwpath = find_path(reg_mingw', 'InstallLocation', 'c:\mingw');
  mingwpath = lower(mingwpath);
  gfortpath = locate('gfortran.exe');
  g95path = locate('g95.exe');
  if isempty(mingwpath) & ~isempty(gfortpath) ...
      & exist([gfortpath '\dlltool.exe'], 'file') ...
      & exist([gfortpath '\nm.exe'], 'file') ...
      & exist([gfortpath '\as.exe'], 'file') ...
      & exist([gfortpath '\gcc.exe'], 'file') mingwpath = fileparts(gfortpath);
  end
  if isempty(g95path) & ~isempty(mingwpath)
    loc = mingwpath;
    if exist(fullfile(loc,'g95.exe'),'file') g95path = loc; end
  end
  if isempty(g95path) & ~isempty(cygwinpath)
    loc = fullfile(fileparts(cygwinpath), 'usr', 'bin');
    if exist(fullfile(loc, 'g95.exe'), 'file') g95path = loc; end
  end
  if isempty(gfortpath)
    loc = adddriveletter('\program files\gfortran\bin');
    if exist(fullfile(loc, 'gfortran.exe'), 'file') gfortpath = loc; end
  end
  if isempty(gfortpath) & ~isempty(cygwinpath)
    loc = adddriveletter('\cygwin\usr\local\gfortran\bin');
    if exist(fullfile(loc, 'gfortran.exe'), 'file') gfortpath = loc; end
  end

function folder = find_path(subkey, valname, default)
  % Try to find folder of cygwin / mingw in the registry, else use default
  try
    folder = winqueryreg('HKEY_LOCAL_MACHINE', subkey, valname);
  catch
    % -- look for a user mount
    try
      folder = winqueryreg('HKEY_CURRENT_USER', subkey, valname);
    catch
      folder = default;
    end
  end
  % if folder doesn't containg bin/gcc.exe, make it empty
  gccfullfile = fullfile(folder, 'bin', 'gcc.exe');
  if ~exist(gccfullfile, 'file'), folder = ''; end

function tf = paths_exist(paths)
% Checks if paths exist, returning tf matrix of same size as paths
if ischar(paths)
  paths = cellstr(paths);
end
tf = zeros(size(paths));
for pi = 1:length(paths)
  tf(pi) = exist(paths{pi}, 'file');
end
return
  
function loc = locate(f)  % "which" (look for Windows command in path) -- return folder
  [pathstr,name,ext] = fileparts(f);
  if isempty(ext) | isequal(ext,'.'), ext = '.exe'; end
  f = [name ext];
  path = getenv('path');
  while ~isempty(path)
    [folder, path]=strtok(path,';');
    fullf = fullfile(folder, f);
    d = dir(fullf);
    if ~isempty(d) & ~d(1).isdir(), loc = folder; return, end
  end
  loc = '';

function about(VERSION)
  m=msgbox({['Gnumex version ' VERSION]...
    ,      'Copyright 2000-2007 Free Software Foundation, Inc'...
    ,      'This program is free software; you can redistribute it and/or modify'...
    ,      'it under the terms of the GNU General Public License as published by'...
    ,      'the Free Software Foundation; either version 2 of the License, or'...
    ,      '(at your option) any later version.'...
    ,      ''...
    ,      'Authors:'...
    ,      '   Ton Van Overbeek, Mumit Khan, ' ...
    ,      '   Matthew Brett, Kristján Jónasson' ...
    ,      '   (and possibly others)'...
    ,      'Web pages: '...
    ,      '   http://gnumex.sourceforge.net'...
    ,      '   https://sourceforge.net/projects/gnumex'}...
    ,      'About gnumex');
  uiwait(m);

function errmsg(varargin)
  gui_f = ~isempty(gnumex('find_cygfig'));
  varargin{1} = [upper(varargin{1}) ': '];  
  if gui_f
    w = warndlg(varargin, 'Error');
    uiwait(w);
  else
    error('\n%s\n%s\n%s\n%s', varargin{:});
  end
  
function warnmsg(varargin)
  gui_f = ~isempty(gnumex('find_cygfig'));
  varargin{1} = [upper(varargin{1}) ': '];
  if gui_f
    w = warndlg(varargin, 'Warning');
    uiwait(w);
  else
    warning('\n%s\n%s\n%s\n%s', varargin{:});
  end

function path = adddriveletter(path) % if path starts with \ and not \\ add x:
  if length(path) >= 2 & path(1) == '\' & path(2) ~= '\'
    p = pwd();
    if length(p) >= 2 & p(1) >= 'A' & p(1) <= 'Z' & p(2) == ':'
      path = [p(1:2) path];
    end
  end
