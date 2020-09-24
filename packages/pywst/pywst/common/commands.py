
__all__ = [ 'WSTCommands' ]

_doc="This is the top-level command for the water security toolkit (WST)."
_epilog="""
WST supports a variety of different water security analysis tools.
Different tools are executed as subcommands of this command.  Each
subcommand supports independent command-line options.  Use the -h option
to print details for a subcommand.  For example, type

   wst flushing -h

to print information about the `flushing` subcommand.

By convention, wst subcommands take a single argument that is a YAML
configuration file.  The output of a wst subcommand is a YAML file.
"""

class WSTCommand(object):
    def __init__(self):
        pass

    def execute(self, options):
        pass

    def brief_description(self):
        pass

    def register_options(self, parser):
        pass

    def generate_template(self):
        pass

    def generate_net3_template(self):
        return self.generate_template()

    def generate_template_documentation(self):
        pass



class CommandRegistry(object):
    def __init__(self):
        self._commands = {}

    def register_command(self, name, handler):
        if name in self._commands:
            raise RuntimeError("ERROR: Duplicate WST command '%s'" % (name,))
        self._commands[name] = handler

    def _common_arguments(self):
        _parser = argparse.ArgumentParser(add_help=False)
        _parser.add_argument(
            "-t", "--trace-exceptions", 
            dest="trace", 
            action="store_true",
            default=False,
            help="[debugging] print a full stack trace when WST (or a "
                "command) generates an unexpected exception" )
        _parser.add_argument(
            "--template", 
            dest="template", 
            action="store_true",
            default=False,
            help="[debugging] print a full stack trace when WST (or a "
                "command) generates an unexpected exception" )
        return _parser
      
    def get_parser(self):
        # First, get the options that are valid across ALL commands
        _common = self._common_arguments()

        # The main parser is just a container for:
        #   - the common options
        #   - the list of commands.
        _parser = argparse.ArgumentParser(
            description=doc, 
            epilog=epilog, 
            formatter_class=argparse.RawDescriptionHelpFormatter,
            parents=[_common])
        # Set up the main parser to recognize comands
        _subparsers = _parser.add_subparsers(
            title='commands',
            help=argparse.SUPPRESS)

        # For each registered command, allow it to register its own options
        for name, handler in self._commands.iteritems():
            _subparser = _subparsers.add_subparser(
                name, 
                help=handler.brief_description(),
                parents=[_common] )
            _subparser.set_defaults(command=handler)
            handler.register_options(_subparser)

        return _parser


WSTCommands = CommandRegistry()
