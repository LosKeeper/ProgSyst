#!/bin/sh

PROG="./monshell"
TMP="/tmp/$$"

# teste si un fichier passé en arg est vide
check_empty ()
{
    if [ -s $1 ]; then
        return 0;
    fi

    return 1
}

# teste si une commande a échouée
# - code de retour du pg doit être égal à 0
# - stdout doit être vide
# - stderr doit contenir un message d'erreur
echec()
{
    if [ $1 -ne 0 ]; then
        echo "échec => code de retour != 0"
        return 0
    fi

    if check_empty $TMP/stdout; then
        echo "échec => sortie standard non vide"
        return 0
    fi

    if ! check_empty $TMP/stderr; then
        echo "échec => sortie erreur vide"
        return 0
    fi

    return 1
}

# teste si une commande a réussi
# - code de retour du pg doit être égal à 0
# - stderr doit être vide
# - si $1 == 0 alors stdout doit être vide, non vide sinon
success()
{
    if [ $2 -ne 0 ]; then
        echo "échec => code de retour != 0"
        return 0
    fi

    if [ $1 -eq 0 ]; then
        if check_empty $TMP/stdout; then
            echo "échec => sortie standard non vide"
            return 0
        fi
    elif ! check_empty $TMP/stdout; then
        echo "échec => sortie standard vide"
        return 0
    fi

    if check_empty $TMP/stderr; then
        echo "échec => sortie erreur non vide"
        return 0
    fi

    return 1
}

success_wstderr()
{
    if [ $1 -ne 0 ]; then
        echo "échec => code de retour != 0"
        return 0
    fi

    if ! check_empty $TMP/stdout; then
        echo "échec => sortie standard vide"
        return 0
    fi

    if ! check_empty $TMP/stderr; then
        echo "échec => sortie erreur vide"
        return 0
    fi

    return 1
}

test_1()
{
    echo "Test 1 - tests sur commandes externes"

    #################################################################################################
    echo -n "Test 1.1 - commande sans option....................."
    pwd > $TMP/pwd
    cat > $TMP/cmd <<EOF
pwd
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;               then                                                  return 1; fi
    if ! cmp $TMP/pwd $TMP/stdout; then echo "échec : résultat de la commande invalide"; return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.2 - commande avec options...................."
    ls -la > $TMP/ls
    cat > $TMP/cmd <<EOF
ls -l -a
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;               then                                                  return 1; fi
    if ! cmp $TMP/ls $TMP/stdout;  then echo "échec : résultat de la commande invalide"; return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.3 - commande inconnue........................"
    cat > $TMP/cmd <<EOF
dflsjkfsjf fdklf fkdf
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if echec $?;                   then                                                  return 1; fi
    echo "OK"
}

test_2()
{
    echo -n "Test 2 - test sur commande exit....................."
    cat > $TMP/cmd <<EOF
exit
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 0 $?;               then                                                  return 1; fi
    echo "OK"
}

test_3()
{
    echo "Test 3 - tests sur commande umask"

    #################################################################################################
    echo -n "Test 3.1 - sans argument............................"
    umask > $TMP/umask
    cat > $TMP/cmd <<EOF
umask
exit
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;                    then                                             return 1; fi
    if ! cmp $TMP/stdout $TMP/umask;    then echo "échec : valeur umask différente";     return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 3.2 - avec argument octal......................"
    cat > $TMP/cmd <<EOF
umask 0555
umask
exit
EOF
    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;                    then                                             return 1; fi
    if [ `cat $TMP/stdout` != "0555" ]; then echo "échec : valeur umask différente";     return 1; fi
    echo "OK"
}

test_4()
{
    echo "Test 4 - tests sur commande cd"

    #################################################################################################
    echo -n "Test 4.1 - avec argument valide....................."
    echo "/usr/bin" > $TMP/cd
    cat > $TMP/cmd <<EOF
cd /usr/bin
pwd
exit
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;                    then                                             return 1; fi
    if ! cmp $TMP/cd $TMP/stdout;       then echo "échec : chgt de rep. non effectif";   return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 4.2 - avec argument invalide..................."
    cat > $TMP/cmd <<EOF
cd fdsmlfjsdlf
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if echec $?;                        then                                             return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 4.3 - sans argument............................"
    echo $HOME > $TMP/cd
    cat > $TMP/cmd <<EOF
cd
pwd
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;                          then                                       return 1; fi
    if ! cmp $TMP/cd $TMP/stdout > /dev/null; then echo "échec : pas dans rép. $HOME";   return 1; fi
    echo "OK"
}

test_5()
{
    echo "Tests 5 - test sur commande print"

    #################################################################################################
    echo -n "Test 5.1 - commande externe qui réussie............."
    cat > $TMP/cmd <<EOF
ls
print
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;               then                                                  return 1; fi
    if [ "0" != `tail -1 $TMP/stdout` ];     then "échec : sortie de print != 0";        return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 5.2 - commande externe qui échoue.............."
    cat > $TMP/cmd <<EOF
msdlkf
print
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success_wstderr $?;                   then                                        return 1; fi
    if [ "1" != `tail -1 $TMP/stdout` ];     then "échec : sortie de print != 1";        return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 5.3 - commande interne qui réussie............."
    cat > $TMP/cmd <<EOF
umask
print
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success 1 $?;               then                                                  return 1; fi
    if [ "0" != `tail -1 $TMP/stdout` ];     then "échec : sortie de print != 0";        return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 5.4 - commande interne qui échoue.............."
    cat > $TMP/cmd <<EOF
cd flkfjl
print
EOF

    $PROG > $TMP/stdout 2> $TMP/stderr < $TMP/cmd
    if success_wstderr $?;                   then                                        return 1; fi
    if [ "1" != `tail -1 $TMP/stdout` ];     then "échec : sortie de print != 1";        return 1; fi
    echo "OK"
}

test_6()
{
    echo -n "Test 6 - test mémoire..............................."
    cat > $TMP/cmd <<EOF
cd
print
ls -l -a
umask
print
pwd
print
exit
EOF
    valgrind --leak-check=full --error-exitcode=100 $PROG >/dev/null 2> $TMP/stderr < $TMP/cmd
    test $? = 100 && echo "échec => log de valgrind dans $TMP/stderr" && return 1
    echo "OK"

    return 0
}

run_all()
{
    # Lance la série des tests
    for T in $(seq 1 6); do
        if test_$T; then
            echo "== Test $T : ok $T/6\n"
        else
            echo "== Test $T : échec"
            return 1
        fi
    done

    rm -R $TMP
}

# répertoire temp où sont stockés tous les fichiers et sorties du pg
mkdir $TMP

if [ $# -eq 1 ]; then
    case $1 in 1) test_1;;
               2) test_2;;
               3) test_3;;
               4) test_4;;
               5) test_5;;
               6) test_6;;
               *) echo "test inexistant"; return 1;
    esac
else
    run_all
fi
