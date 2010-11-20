/* *************************************************************************
 *  Copyright 2010 Jakob Gruber                                            *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ************************************************************************* */

#include "program.h"

Program::Program()
{
    quit = false;
    op = OP_NONE;
    rightpane = RPE_INFO;
    mode = MODE_STANDARD;
    sortedby = A_NAME;
}
Program::~Program() {
    deinit_curses();

    for (unsigned int i = 0; i < filteredpackages.size(); i++)
        delete filteredpackages[i];

    if (alpm_release() != 0)
        throw AlpmException("failed to deinitialize alpm library");
}

void Program::Init() {

    std::cout << "Reading package dbs, please wait..." << std::endl;

    /* initialize libalpm */
    init_alpm();

    /* create our package list */
    alpm_list_t *dbs = alpm_list_copy(alpm_option_get_syncdbs());
    dbs = alpm_list_add(dbs, localdb);
    for ( ; dbs; dbs = alpm_list_next(dbs)) {
        pmdb_t *db = (pmdb_t*)alpm_list_getdata(dbs);
        for (alpm_list_t *pkgs = alpm_db_get_pkgcache(db); pkgs; pkgs = alpm_list_next(pkgs)) {
            pmpkg_t *pkg = (pmpkg_t*)alpm_list_getdata(pkgs);
            Package *p = new Package(pkg);
            Package *parray[] = { p };
            if (!std::includes(packages.begin(), packages.end(),
                              parray, parray + 1,
                              boost::bind(&Filter::cmp, _1, _2, A_NAME))) {
                packages.push_back(p);
            }
            else
                delete p;
        }
        std::sort(packages.begin(), packages.end(), boost::bind(&Filter::cmp, _1, _2, A_NAME));
    }
    alpm_list_free(dbs);

    filteredpackages = packages;

    /* initialize curses */
    system("clear");
    init_curses();
    focused_pane = list_pane;
    list_pane->SetList(&filteredpackages);
    queue_pane->SetList(&opqueue);
    updatelistinfo();
    updatedisplay();
}

void Program::MainLoop() {
    int ch;
    while (!quit) {
        ch = getch();

        if (mode == MODE_STANDARD) {
            switch (ch) {
            case KEY_UP:
                focused_pane->Move(-1);
                break;
            case KEY_DOWN:
                focused_pane->Move(1);
                break;
            case KEY_HOME:
                focused_pane->MoveAbs(0);
                break;
            case KEY_END:
                focused_pane->MoveAbs(filteredpackages.size() - 1);
                break;
            case KEY_PPAGE:   /* page up */
                focused_pane->Move(list_pane->UsableHeight() * -1);
                break;
            case KEY_NPAGE:   /* page down */
                focused_pane->Move(list_pane->UsableHeight());
                break;
            case KEY_TAB:
                focused_pane = (focused_pane == list_pane) ?
                               queue_pane : list_pane;
                break;
            case KEY_SPACE:
                rightpane = (rightpane == RPE_INFO) ? RPE_QUEUE : RPE_INFO;
                break;
            case KEY_RIGHT:
                if (focused_pane != list_pane) break;
                if (filteredpackages.size() == 0) break;
                opqueue.push_back(filteredpackages[list_pane->FocusedIndex()]);
                break;
            case KEY_LEFT:
                /* TODO */
                break;
            case 'h':
                mode = MODE_HELP;
                break;
            case 'q':
                quit = true;
                break;
            case 'c':
                clearfilter();
                break;
            case 'n':
            case 'd':
                mode = MODE_INPUT;
                inputbuf = ch;
                inputbuf += ":";
                op = OP_FILTER;
                break;
            case '/':
                mode = MODE_INPUT;
                inputbuf = "";
                op = OP_FILTER;
                break;
            case '.':
                mode = MODE_INPUT;
                inputbuf = "";
                op = OP_SORT;
                break;
            default:
                break;
            }
        } else if (mode == MODE_INPUT) {
            switch (ch) {
            case KEY_ESC:
                mode = MODE_STANDARD;
                op = OP_NONE;
                break;
            case KEY_RETURN:
                mode = MODE_STANDARD;
                if (op == OP_FILTER) {
                    displayprocessingmsg();
                    filterpackages(inputbuf);
                    list_pane->SetList(&filteredpackages);
                    flushinp();
                } else if (op == OP_SORT) {
                    sortpackages(inputbuf);
                }
                op = OP_NONE;
                break;
            case KEY_BACKSPACE:
            case KEY_KONSOLEBACKSPACE:
                if (inputbuf.length() > 0)
                    inputbuf = inputbuf.substr(0, inputbuf.length() - 1);
                break;
            default:
                inputbuf += ch;
                break;
            }
        } else if (mode == MODE_HELP) {
            switch (ch) {
            case KEY_ESC:
            case KEY_RETURN:
            case KEY_SPACE:
                mode = MODE_STANDARD;
                break;
            default:
                break;
            }
        }

        updatedisplay();
    }
}

void Program::displayprocessingmsg() {
    list_pane->SetFooter("Processing...");
    updatedisplay();
}

#define PRINTH(a, b) help_pane->PrintW(a, A_BOLD); help_pane->PrintW(b);
void Program::print_help() {
    help_pane->PrintW("COMMANDS\n", A_BOLD);
    help_pane->PrintW("\n");
    PRINTH("ESC: ", "cancel\n");
    PRINTH("q: ", "quit\n");
    PRINTH("/: ", "filter packages by specified fields (using regexp)\n");
    PRINTH("", "   note that filters can be chained.\n")
    PRINTH("n: ", "filter packages by name (using regexp)\n");
    PRINTH("c: ", "clear all package filters\n");
    PRINTH(".: ", "sort packages specified field\n");
    PRINTH("arrows, pg up/down, home/end: ", "navigation\n");
    PRINTH("return: ", "exit help\n");
}
#undef PRINTH

void Program::init_alpm() {
    if (alpm_initialize() != 0)
        throw AlpmException("failed to initialize alpm library");

    Config conf;
    conf.parse();

    alpm_option_set_dbpath(conf.getDBPath().c_str());
    alpm_option_set_logfile(conf.getLogFile().c_str());
    alpm_option_set_root(conf.getRootDir().c_str());

    vector<string> repos = conf.getRepos();
    for (unsigned int i = 0; i < repos.size(); i++)
        alpm_db_register_sync(repos[i].c_str());

    alpm_db_register_local();
    localdb = alpm_option_get_localdb();
}

void Program::init_curses() {

    setlocale(LC_ALL, "");

    initscr();
    start_color();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    noecho();

    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_WHITE);

    const int lwidth = 30;
    const int height = LINES - 1;
    list_pane = new CursesListBox(lwidth, height, 0, 0, true);
    info_pane = new CursesFrame(COLS - lwidth + 1, height, 0, lwidth - 1, true);
    queue_pane = new CursesListBox(COLS - lwidth + 1, height, 0, lwidth - 1, true);
    status_pane = new CursesFrame(COLS, 1, LINES - 1, 0, false);
    input_pane = new CursesFrame(COLS, 3, LINES - 2, 0, true);
    help_pane = new CursesFrame(COLS - 10, LINES - 10, 5, 5, true);

    list_pane->SetHeader("Packages");
    info_pane->SetHeader("Info");
    info_pane->SetFooter("Press h for help");
    queue_pane->SetHeader("Queue");
    input_pane->SetHeader("Input");
    help_pane->SetHeader("Help");

    status_pane->SetBackground(C_INV);
}
void Program::deinit_curses() {
    delete list_pane;
    delete queue_pane;
    delete info_pane;
    delete status_pane;
    delete input_pane;
    delete help_pane;

    nocbreak();
    curs_set(1);
    echo();

    endwin();
}

void Program::printinfosection(AttributeEnum attr, string text) {

    string caption = AttributeInfo::attrname(attr);
    char hllower = AttributeInfo::attrtochar(attr);
    char hlupper = toupper(hllower);
    bool hldone = false;
    int style;

    for (unsigned int i = 0; i < caption.size(); i++) {
        if (!hldone && (caption[i] == hllower || caption[i] == hlupper)) {
            style = C_DEF;
            hldone = true;
        }
        else style = C_DEF_HL2;


        info_pane->PrintW(string(1, caption[i]), style);
    }
    info_pane->PrintW(": ", C_DEF_HL2);

    string txt = text + "\n";
    info_pane->PrintW(txt);
}
void Program::updatedisplay() {

    /* this runs **at least** once per loop iteration
       for example it can run more than once if we need to display
       a 'processing' message during filtering
     */

    if (mode == MODE_INPUT || mode == MODE_STANDARD) {
        Package *pkg;

        clear();
        list_pane->Clear();
        info_pane->Clear();
        status_pane->Clear();
        input_pane->Clear();
        queue_pane->Clear();

        /* info pane */
        if ((unsigned int)(list_pane->FocusedIndex()) < filteredpackages.size()) {
            pkg = filteredpackages[list_pane->FocusedIndex()];
            for (int i = 0; i < A_NONE; i++) {
                AttributeEnum attr = (AttributeEnum)i;
                string txt = pkg->getattr(attr);
                if (txt.length() != 0)
                    printinfosection(attr, txt);
            }
        }

        /* status bar */
        status_pane->MvPrintW(1, 0, "Sorted by: ", C_INV_HL1);
        status_pane->PrintW(AttributeInfo::attrname(sortedby), C_INV);
        status_pane->PrintW(" Filtered by: ", C_INV_HL1);
        status_pane->PrintW(((searchphrases.length() == 0) ? "-" : searchphrases), C_INV);

        refresh();
        status_pane->Refresh();
        list_pane->Refresh();
        if (rightpane == RPE_INFO)
            info_pane->Refresh();
        else
            queue_pane->Refresh();

        if (mode == MODE_INPUT) {
            input_pane->PrintW(optostr(op) + inputbuf);
            input_pane->Refresh();
        }
    } else if (mode == MODE_HELP) {
        help_pane->Clear();
        print_help();
        help_pane->Refresh();
    }
}

string Program::optostr(FilterOperationEnum o) const {
    switch (o) {
    case OP_FILTER: return "/";
    case OP_SORT: return ".";
    case OP_NONE: return "";
    default: assert(0);
    }
}

void Program::clearfilter() {
    filteredpackages = packages;
    searchphrases = "";
    updatelistinfo();
}

void Program::sortpackages(string str) {
    if (str.length() < 1)
        return;

    AttributeEnum attr = A_NONE;
    unsigned int i = 0;

    while (attr == A_NONE && i < str.length()) {
        attr = AttributeInfo::chartoattr(str[i]);
        i++;
    }

    if (attr == A_NONE)
        return;

    sortedby = attr;

    std::sort(filteredpackages.begin(), filteredpackages.end(),
              boost::bind(&Filter::cmp, _1, _2, attr));
}

void Program::filterpackages(string str) {

    string fieldlist, searchphrase;

    /* first, split actual search phrase from field prefix */
    sregex reprefix = sregex::compile("^([A-Za-z]*):(.*)");
    smatch what;

    Filter::clearattrs();
    if (regex_search(str, what, reprefix)) {
        fieldlist = what[1];
        searchphrase = what[2];
        Filter::setattrs(fieldlist);
    } else {
        searchphrase = str;
    }

    /* if search phrase is empty, nothing to do */
    if (searchphrase.length() == 0) {
        updatelistinfo();
        return;
    }

    if (searchphrases.length() != 0) searchphrases += ", ";
    searchphrases += str;

    /* if search phrase is alphanumeric only,
       perform a fast and simple search, else run slower regexp search */

    sregex resimple = sregex::compile("[:alnum:]+");

    if (regex_match(searchphrase, what, resimple)) {
        for (vector<Package*>::iterator it = std::find_if(filteredpackages.begin(),
                                                     filteredpackages.end(),
                                                     boost::bind(&Filter::matches, _1, searchphrase));
            it != filteredpackages.end();
            it = std::find_if(filteredpackages.begin(),
                              filteredpackages.end(),
                              boost::bind(&Filter::matches, _1, searchphrase)))
            filteredpackages.erase(it);
    } else {
        sregex needle = sregex::compile(searchphrase, icase);
        for (vector<Package*>::iterator it = std::find_if(filteredpackages.begin(),
                                                     filteredpackages.end(),
                                                     boost::bind(&Filter::matchesre, _1, needle));
            it != filteredpackages.end();
            it = std::find_if(filteredpackages.begin(),
                              filteredpackages.end(),
                              boost::bind(&Filter::matchesre, _1, needle)))
            filteredpackages.erase(it);
    }

    updatelistinfo();
}
void Program::updatelistinfo() {
    list_pane->SetFooter(boost::str(boost::format("%d Package(s)") % filteredpackages.size()));
}
